#pragma once

#include "utils/MysqlUtil.h"

#include "router/RouterHandler.h"
#include "utils/ParseJsonUtil.h"
#include "ChatServer.h"

class ChatSendHandler : public http::router::RouterHandler
{
public:
	ChatSendHandler(ChatServer* server) :server_(server) {}
	void handle(const http::HttpRequest& req, http::HttpResponse* resp) override;

private:
	ChatServer* server_;
	http::MysqlUtil mysqlUtil_;
};