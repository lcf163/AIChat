#include "http/HttpServer.h"
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"
#include <iostream>
#include <string>

using namespace http;

int main() {
    try {
        std::cout << "HTTP Server Demo" << std::endl;
        std::cout << "=================" << std::endl;
        
        // 创建HTTP服务器，监听8080端口
        HttpServer server(8080, "HttpServerDemo", false);
        
        // 设置线程数
        server.setThreadNum(4);
        
        // 注册根路径处理器
        server.Get("/", [](const http::HttpRequest& req, http::HttpResponse* resp) {
            resp->setVersion("HTTP/1.1");
            resp->setStatusCode(http::HttpResponse::k200Ok);
            resp->setStatusMessage("OK");
            resp->setContentType("text/html");
            resp->setBody("<html><body><h1>Welcome to HttpServer Demo!</h1><p>This is a simple HTTP server using HttpServer framework.</p></body></html>");
        });
        
        // 注册API路径处理器
        server.Get("/api/hello", [](const http::HttpRequest& req, http::HttpResponse* resp) {
            resp->setVersion("HTTP/1.1");
            resp->setStatusCode(http::HttpResponse::k200Ok);
            resp->setStatusMessage("OK");
            resp->setContentType("application/json");
            resp->setBody("{\"message\": \"Hello from HttpServer!\", \"status\": \"success\"}");
        });
        
        // 注册POST处理器
        server.Post("/api/data", [](const http::HttpRequest& req, http::HttpResponse* resp) {
            std::string requestBody = req.getBody();
            
            resp->setVersion("HTTP/1.1");
            resp->setStatusCode(http::HttpResponse::k200Ok);
            resp->setStatusMessage("OK");
            resp->setContentType("application/json");
            
            std::string response = "{\"received_data\": \"" + requestBody + "\", \"status\": \"processed\"}";
            resp->setBody(response);
        });
        
        // 启动服务器
        std::cout << "Starting HTTP server on port 8080..." << std::endl;
        std::cout << "Available endpoints:" << std::endl;
        std::cout << "  GET  http://localhost:8080/" << std::endl;
        std::cout << "  GET  http://localhost:8080/api/hello" << std::endl;
        std::cout << "  GET  http://localhost:8080/api/user/:id" << std::endl;
        std::cout << "  POST http://localhost:8080/api/data" << std::endl;
        std::cout << "\nPress Ctrl+C to stop the server..." << std::endl;
        
        server.start();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}