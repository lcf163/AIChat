
#include "handlers/ChatRegisterHandler.h"

void ChatRegisterHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp)
{
    json parsed;
    if (!ParseJsonUtil::parseJsonFromBody(req, resp, parsed)) {
        // 错误响应已经在parseJsonFromBody中设置
        return;
    }
    
    std::string username = parsed["username"];
    std::string password = parsed["password"];

    int userId = insertUser(username, password);
    if (userId != -1)
    {
        json successResp;
        successResp["status"] = "success";
        successResp["message"] = "Register successful";
        successResp["userId"] = userId;
        std::string successBody = successResp.dump(4);

        resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
        resp->setCloseConnection(false);
        resp->setContentType("application/json");
        resp->setContentLength(successBody.size());
        resp->setBody(successBody);
    }
    else
    {
        json failureResp;
        failureResp["status"] = "error";
        failureResp["message"] = "username already exists";
        std::string failureBody = failureResp.dump(4);

        resp->setStatusLine(req.getVersion(), http::HttpResponse::k409Conflict, "Conflict");
        resp->setCloseConnection(false);
        resp->setContentType("application/json");
        resp->setContentLength(failureBody.size());
        resp->setBody(failureBody);
    }
}

int ChatRegisterHandler::insertUser(const std::string& username, const std::string& password)
{
    // 生成盐值
    std::string salt = PasswordUtil::generateSalt();
    
    // 使用PBKDF2算法对密码进行哈希
    std::string hashedPassword = PasswordUtil::hashPassword(password, salt);
    
    // 使用 INSERT IGNORE 来避免竞态条件
    // 如果用户名已存在，插入会被忽略，返回0行受影响
    std::string sql = "INSERT IGNORE INTO users (username, password, salt) VALUES (?, ?, ?)";
    int affectedRows = mysqlUtil_.executeUpdate(sql, username, hashedPassword, salt);
    
    // 如果插入成功（影响行数大于0），则获取用户ID
    if (affectedRows > 0) {
        std::string sql2 = "SELECT id FROM users WHERE username = ?";
        auto res = mysqlUtil_.executeQuery(sql2, username);
        if (res->next())
        {
            return res->getInt("id");
        }
    }
    
    // 插入失败或被忽略，说明用户名已存在
    return -1;
}

bool ChatRegisterHandler::isUserExist(const std::string& username)
{
    std::string sql = "SELECT id FROM users WHERE username = ?";
    auto res = mysqlUtil_.executeQuery(sql, username);
    if (res->next())
    {
        return true;
    }
    return false;
}