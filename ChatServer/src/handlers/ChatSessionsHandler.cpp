#include "handlers/ChatSessionsHandler.h"
#include <muduo/base/Logging.h>
#include <shared_mutex>

void ChatSessionsHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp)
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
        
        json successResp;
        successResp["success"] = true;
        json sessionArray = json::array();

        // 优先从内存查询会话信息
        std::vector<std::string> sessionsFromMemory;
        {
            // 使用无锁方法替代 mutexForSessionsId
            sessionsFromMemory = server_->getSessionIds(userId);
        }

        // 如果内存中有会话数据，直接使用
        if (!sessionsFromMemory.empty()) {
            for (auto sid : sessionsFromMemory) {
                json s;
                s["sessionId"] = sid;
                s["name"] = "会话 " + sid;
                sessionArray.push_back(s);
            }
            LOG_INFO << "Found " << sessionsFromMemory.size() << " sessions in memory";
        } else {
            // 如果内存中没有会话数据，从数据库查询
            LOG_INFO << "No sessions in memory, querying from database...";
            try {
                std::string sql = "SELECT session_id, title, created_at, updated_at FROM chat_session WHERE user_id = ? ORDER BY updated_at DESC";
                
                std::unique_ptr<sql::ResultSet> result(server_->mysqlUtil_.executeQuery(sql, std::to_string(userId)));
                
                if (result) {
                    while (result->next()) {
                        json s;
                        s["sessionId"] = result->getString("session_id");
                        s["name"] = result->getString("title");
                        s["createdAt"] = result->getString("created_at");
                        s["updatedAt"] = result->getString("updated_at");
                        sessionArray.push_back(s);
                        
                        // 同时将查询到的会话信息加载到内存中
                        {
                            // 使用无锁方法替代 mutexForSessionsId
                            server_->addSessionId(userId, result->getString("session_id"));
                        }
                    }
                    // unique_ptr会自动释放资源，不再需要手动delete
                    LOG_INFO << "Loaded " << sessionArray.size() << " sessions from database";
                }
            } catch (const std::exception& e) {
                LOG_ERROR << "Failed to query sessions from database: " << e.what();
            }
        }

        successResp["sessions"] = sessionArray;
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