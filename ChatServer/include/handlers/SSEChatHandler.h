#pragma once

#include <unordered_map>
#include <mutex>
#include <memory>
#include <muduo/net/TcpConnection.h>

#include "utils/JsonUtil.h"

#include "router/RouterHandler.h"
#include "ChatServer.h"

class SSEChatHandler : public http::router::RouterHandler
{
public:	
	// 超时检查相关
	static const int SSE_TIMEOUT_SECONDS = 30;

	SSEChatHandler(ChatServer* server) : server_(server) {}
		void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;

private:
	ChatServer* server_;
	
};