#pragma once

#include "utils/MysqlUtil.h"

#include "router/RouterHandler.h"
#include "utils/PasswordUtil.h"
#include "utils/ParseJsonUtil.h"
#include "ChatServer.h"

class ChatRegisterHandler : public http::router::RouterHandler
{
public:
    explicit ChatRegisterHandler(ChatServer* server) : server_(server) {}

    void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;

private:
    int insertUser(const std::string& username, const std::string& password);
    bool isUserExist(const std::string& username);

    ChatServer* server_;
    http::MysqlUtil mysqlUtil_;
};