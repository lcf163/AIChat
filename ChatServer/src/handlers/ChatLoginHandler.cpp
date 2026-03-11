#include "handlers/ChatLoginHandler.h"
#include <shared_mutex>

void ChatLoginHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp)
{ 
    json parsed;
    if (!ParseJsonUtil::parseJsonFromBody(req, resp, parsed)) {
        // 错误响应已经在parseJsonFromBody中设置
        return;
    }

    try
    {
        std::string username = parsed["username"];
        std::string password = parsed["password"];

        int userId = queryUserId(username, password);
        if (userId != -1)
        {
            auto session = server_->getSessionManager()->getSession(req, resp);
            session->setValue("userId", std::to_string(userId));
            session->setValue("username", username);
            session->setValue("isLoggedIn", "true");

            // 检查在线状态并更新
            // 使用无锁方法替代 mutexForOnlineUsers_
            if (!server_->isUserOnline(userId))
            {
                server_->addUser(userId);

                json successResp;
                successResp["success"] = true;
                successResp["userId"] = userId;
                std::string successBody = successResp.dump(4);

                resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
                resp->setCloseConnection(false);
                resp->setContentType("application/json");
                resp->setContentLength(successBody.size());
                resp->setBody(successBody);
                return;
            }
            else
            {
                json failureResp;
                failureResp["success"] = false;
                failureResp["error"] = "账号已在其他地方登录";
                std::string failureBody = failureResp.dump(4);

                resp->setStatusLine(req.getVersion(), http::HttpResponse::k403Forbidden, "Forbidden");
                resp->setCloseConnection(true);
                resp->setContentType("application/json");
                resp->setContentLength(failureBody.size());
                resp->setBody(failureBody);
                return;
            }
        }
        else 
        {
            json failureResp;
            failureResp["status"] = "error";
            failureResp["message"] = "Invalid username or password";
            std::string failureBody = failureResp.dump(4);

            resp->setStatusLine(req.getVersion(), http::HttpResponse::k401Unauthorized, "Unauthorized");
            resp->setCloseConnection(false);
            resp->setContentType("application/json");
            resp->setContentLength(failureBody.size());
            resp->setBody(failureBody);
            return;
        }
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
        return;
    }
}

int ChatLoginHandler::queryUserId(const std::string& username, const std::string& password)
{
    // 先查询用户的盐值
    std::string saltSql = "SELECT id, password, salt FROM users WHERE username = ?";
    auto res = mysqlUtil_.executeQuery(saltSql, username);
    
    if (res->next())
    {
        int id = res->getInt("id");
        std::string storedPassword = res->getString("password");
        std::string salt = res->getString("salt");
        
        // 验证密码
        if (PasswordUtil::verifyPassword(password, salt, storedPassword))
        {
            return id;
        }
    }

    return -1;
}