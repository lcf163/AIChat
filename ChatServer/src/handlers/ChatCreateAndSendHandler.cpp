#include "handlers/ChatCreateAndSendHandler.h"
#include <muduo/base/Logging.h>

void ChatCreateAndSendHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp)
{
	try
	{
		auto session = server_->getSessionManager()->getSession(req, resp);
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
		std::string modelType;

		json j;
		if (!ParseJsonUtil::parseJsonFromBody(req, resp, j)) {
			// 错误响应已经在parseJsonFromBody中设置
			return;
		}
		
		if (!j.empty()) {
			if (j.contains("question")) userQuestion = j["question"];

			modelType = j.contains("modelType") ? j["modelType"].get<std::string>() : "1";
		}

		AISessionIdGenerator generator;
		std::string sessionId = generator.generate();

		// 异步持久化会话到数据库，避免阻塞主线程
		server_->getBusinessThreadPool()->enqueue([this, userId, username, sessionId]() {
			try {
				// 使用参数化查询防止SQL注入
				std::string insertSessionSql = "INSERT INTO chat_session (user_id, username, session_id, title) VALUES (?, ?, ?, ?)";
				mysqlUtil_.executeUpdate(insertSessionSql, std::to_string(userId), username, sessionId, "新对话");
			} catch (const std::exception& e) {
				LOG_ERROR << "Failed to persist session to database: " << e.what();
			}
		});

		std::shared_ptr<AIHelper> AIHelperPtr;
		{
			// 使用无锁方法替代 mutexForChatInformation
			AIHelperPtr = server_->getChatSession(userId, sessionId);
			
			// 如果不存在则创建新实例
			if (!AIHelperPtr) {
				AIHelperPtr = std::make_shared<AIHelper>();
				server_->addOrUpdateChatSession(userId, sessionId, AIHelperPtr);
				// 同时将新会话ID添加到内存中
				server_->addSessionId(userId, sessionId);
			}
			
			// 更新LRU缓存
			server_->updateLRUCache(userId, sessionId);
		}

		// 使用线程池异步处理AI请求，真正避免阻塞IO线程
		auto future = AIHelperPtr->chatAsync(server_->getBusinessThreadPool(), userId, username, sessionId, userQuestion, modelType);
		
		// 在另一个线程中等待结果并处理
		server_->getBusinessThreadPool()->enqueue([this, sessionId, userId, future = std::move(future)]() mutable {
			try {
				std::string result = future.get();
				// 通过SSE推送结果给客户端
				server_->sendSSEData(sessionId, result, "result");
                // 发送结束信号
                server_->sendSSEData(sessionId, "{\"status\":\"done\"}", "end");
			} catch (const std::exception& e) {
				LOG_ERROR << "AI task failed for session " << sessionId << ": " << e.what();
                // 发送错误事件
                server_->sendSSEData(sessionId, "{\"error\":\"Processing Failed\"}", "error");
			}
		});

		// 立即返回 sessionId，前端需通过SSE获取结果
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
		failureResp["status"] = "error";
		failureResp["message"] = e.what();
		std::string failureBody = failureResp.dump(4);
		resp->setStatusLine(req.getVersion(), http::HttpResponse::k400BadRequest, "Bad Request");
		resp->setCloseConnection(true);
		resp->setContentType("application/json");
		resp->setContentLength(failureBody.size());
		resp->setBody(failureBody);
	}
}