#pragma once

#include "utils/MysqlUtil.h"

#include "router/RouterHandler.h"
#include "utils/ParseJsonUtil.h"
#include "core/AISessionIdGenerator.h"
#include "ChatServer.h"

class ChatCreateAndSendHandler : public http::router::RouterHandler
{
public:
	ChatCreateAndSendHandler(ChatServer* server) :server_(server) {}
	void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;

private:
	ChatServer* server_;
	http::MysqlUtil mysqlUtil_;
};