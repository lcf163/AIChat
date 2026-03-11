#include "handlers/ChatSendHandler.h"
#include <muduo/base/Logging.h>

void ChatSendHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp)
{
	try
	{
		auto session = server_->getSessionManager()->getSession(req, resp);
		LOG_INFO << "session->getValue(\"isLoggedIn\") = " << session->getValue("isLoggedIn");
		if (session->getValue("isLoggedIn") != "true")
		{
			json errorResp;
			errorResp["status"] = "error";
			errorResp["message"] = "Unauthorized";
			std::string errorBody = errorResp.dump(4);

			server_->packageResp(req.getVersion(), http::HttpResponse::k401Unauthorized,
				"Unauthorized", true, "application/json", errorBody.size(),
				errorBody, resp);
			return;
		}

		int userId = std::stoi(session->getValue("userId"));
		std::string username = session->getValue("username");
		std::string userQuestion;
		std::string sessionId;
		std::string modelType;

		json j;
		if (!ParseJsonUtil::parseJsonFromBody(req, resp, j)) {
			// 错误响应已经在parseJsonFromBody中设置
			return;
		}
		
		if (j.contains("question")) userQuestion = j["question"];
		if (j.contains("sessionId")) sessionId = j["sessionId"];

		modelType = j.contains("modelType") ? j["modelType"].get<std::string>() : "1";

		std::shared_ptr<AIHelper> AIHelperPtr;
		{
			// 使用无锁方法替代 mutexForChatInformation
			AIHelperPtr = server_->getChatSession(userId, sessionId);
			
			// 如果不存在则创建新实例
			if (!AIHelperPtr) {
				AIHelperPtr = std::make_shared<AIHelper>();
				server_->addOrUpdateChatSession(userId, sessionId, AIHelperPtr);
			}
			
			// 更新LRU缓存
			server_->updateLRUCache(userId, sessionId);
		}

		// 更新会话时间戳
		try {
			// 使用参数化查询防止SQL注入
			std::string updateSessionSql = "UPDATE chat_session SET updated_at = CURRENT_TIMESTAMP WHERE session_id = ?";
			mysqlUtil_.executeUpdate(updateSessionSql, sessionId);
		} catch (const std::exception& e) {
			LOG_ERROR << "Failed to update session timestamp: " << e.what();
			// 继续执行，即使数据库操作失败
		}

        // 定义流式回调：收到一个字，就发给前端
        auto streamCallback = [this, sessionId](const std::string& chunk) {
            // chunk 是解析好的纯文本
            if (!chunk.empty()) {
                server_->sendSSEData(sessionId, chunk, "result");
            }
        };

		// 使用线程池异步处理AI请求，真正避免阻塞IO线程
		auto future = AIHelperPtr->chatAsync(server_->getBusinessThreadPool(), userId, username, sessionId, userQuestion, modelType, streamCallback);
		
		// 在另一个线程中等待结果并处理最终结果
		server_->getBusinessThreadPool()->enqueue([this, sessionId, userId, future = std::move(future)]() mutable {
			try {
				std::string result = future.get();
				// 推送结果给客户端 (这里是 SSE)
				{
					// 使用无锁方法替代 mutexForChatResults
					server_->setChatResult(sessionId, result);
				}
				
				// 【关键】发送结束信号给前端
				server_->sendSSEData(sessionId, "{\"status\":\"done\"}", "end");
				
			} catch (const std::exception& e) {
				LOG_ERROR << "AI task failed for session " << sessionId << ": " << e.what();
				// 发送错误事件
				server_->sendSSEData(sessionId, "{\"error\":\"Processing Failed\"}", "error");
			}
		});

		// 立即返回成功响应，前端通过 SSE 接收后续数据
		json successResp;
		successResp["success"] = true;
		successResp["message"] = "AI processing started";
		successResp["sessionId"] = sessionId;

		std::string successBody = successResp.dump(4);
		resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
		resp->setCloseConnection(false);
		resp->setContentType("application/json");
		resp->setContentLength(successBody.size());
		resp->setBody(successBody);
		return;
	}
	catch (const std::exception& e)
	{
		json failureResp;
		failureResp["success"] = false;
		failureResp["error"] = e.what();
		std::string failureBody = failureResp.dump(4);

		resp->setStatusLine(req.getVersion(), http::HttpResponse::k400BadRequest, "Bad Request");
		resp->setCloseConnection(true);
		resp->setContentType("application/json");
		resp->setContentLength(failureBody.size());
		resp->setBody(failureBody);
		return;
	}
}