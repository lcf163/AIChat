#include "handlers/SSEChatHandler.h"
#include <muduo/base/Logging.h>
#include <muduo/net/Buffer.h>
#include <sstream>

void SSEChatHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp)
{
    try
    {
        auto session = server_->getSessionManager()->getSession(req, resp);
        if (session->getValue("isLoggedIn") != "true")
        {
            resp->setStatusLine(req.getVersion(), http::HttpResponse::k401Unauthorized, "Unauthorized");
            resp->setCloseConnection(true);
            return;
        }

        std::string sessionId = req.getQueryParameters("sessionId");

        if (sessionId.empty()) {
            resp->setStatusLine(req.getVersion(), http::HttpResponse::k400BadRequest, "Bad Request");
            resp->setCloseConnection(true);
            return;
        }

        // 设置SSE响应头
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k200Ok, "OK");
        resp->setCloseConnection(false);
        resp->addHeader("Content-Type", "text/event-stream");
        resp->addHeader("Cache-Control", "no-cache");
        resp->addHeader("Connection", "keep-alive");
        resp->addHeader("Access-Control-Allow-Origin", "*");
        resp->addHeader("X-Accel-Buffering", "no"); // 禁用nginx缓冲
        
        // 启用流式响应模式
        resp->setStreamingMode(true);
        resp->setStreamWriteCallback([this, sessionId](muduo::net::TcpConnectionPtr conn, http::HttpResponse* r) {
            // 注册连接
            server_->addSSEConnection(sessionId, conn);
            
            // 发送初始连接确认事件
            std::ostringstream initData;
            initData << "event: connected\n";
            initData << "data: {\"sessionId\": \"" << sessionId << "\", \"status\": \"connected\"}\n\n";
            
            conn->send(initData.str());
            
            
            return false; // 返回false停止HttpServer的驱动循环，但连接保持打开
        });
    }
    catch (const std::exception& e)
    {
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k500InternalServerError, "Internal Server Error");
        resp->setCloseConnection(true);
    }
}