#include "handlers/ChatHistoryHandler.h"
#include <muduo/base/Logging.h>

void ChatHistoryHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp)
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

        std::string sessionId;
        json j;
        if (!ParseJsonUtil::parseJsonFromBody(req, resp, j)) {
            // 错误响应已经在parseJsonFromBody中设置
            return;
        }
        
        if (!j.empty()) {
            if (j.contains("sessionId")) sessionId = j["sessionId"];
        }
        
        json historyArray = json::array();

        // 直接从数据库查询历史记录
        try {
            std::string sql = "SELECT is_user, content FROM chat_message "
                          "WHERE user_id = ? AND session_id = ? "
                          "ORDER BY ts ASC, id ASC";

            std::unique_ptr<sql::ResultSet> result(server_->mysqlUtil_.executeQuery(sql, std::to_string(userId), sessionId));
            
            if (result) {
                while (result->next()) {
                    json msgJson;
                    msgJson["is_user"] = (result->getInt("is_user") == 1);
                    msgJson["content"] = result->getString("content");
                    historyArray.push_back(msgJson);
                }
                // unique_ptr会自动释放资源，不再需要手动delete
                LOG_INFO << "Found " << historyArray.size() << " messages for session " << sessionId;
            }
        } catch (const std::exception& e) {
            LOG_ERROR << "Failed to query message history: " << e.what();
        }

        json successResp;
        successResp["success"] = true;
        successResp["history"] = historyArray;
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