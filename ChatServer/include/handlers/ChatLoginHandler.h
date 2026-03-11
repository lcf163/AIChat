#pragma once

#include "utils/MysqlUtil.h"

#include "utils/ParseJsonUtil.h"
#include "utils/PasswordUtil.h"
#include "router/RouterHandler.h"
#include "ChatServer.h"

class ChatLoginHandler : public http::router::RouterHandler
{
public:
    explicit ChatLoginHandler(ChatServer* server) : server_(server) {}

    void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;

private:
    int queryUserId(const std::string& username, const std::string& password);

    ChatServer* server_;
    http::MysqlUtil mysqlUtil_;
};