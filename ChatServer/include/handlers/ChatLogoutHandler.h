#pragma once

#include "router/RouterHandler.h"
#include "utils/JsonUtil.h"

#include "ChatServer.h"

class ChatLogoutHandler : public http::router::RouterHandler
{
public:
    explicit ChatLogoutHandler(ChatServer* server) : server_(server) {}
    void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;

private:
    ChatServer* server_;
};
