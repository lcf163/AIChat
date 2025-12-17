#pragma once

#include "router/RouterHandler.h"
#include "core/AIConfig.h"
#include "ChatServer.h"

// 前置声明
class SpeechService;

class ChatSpeechHandler : public http::router::RouterHandler
{
public:
    explicit ChatSpeechHandler(ChatServer* server) : server_(server) {}
    void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;

private:
    ChatServer* server_;
};
