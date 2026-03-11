#include "utils/ParseJsonUtil.h"

bool ParseJsonUtil::parseJsonFromBody(const http::HttpRequest& req, http::HttpResponse* resp, json& outJson) {
    // 检查Content-Type头部和请求体
    auto contentType = req.getHeader("Content-Type");
    std::string body = req.getBody();
    
    // 限制请求体大小，防止DoS攻击 (最大1MB)
    const size_t MAX_BODY_SIZE = 1024 * 1024; // 1MB
    if (body.size() > MAX_BODY_SIZE) {
        json errorResp;
        errorResp["success"] = false;
        errorResp["error"] = "Request entity too large: body size exceeds 1MB limit";
        std::string errorBody = errorResp.dump(4);
        
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k400BadRequest, "Request Entity Too Large");
        resp->setCloseConnection(false);
        resp->setContentType("application/json");
        resp->setContentLength(errorBody.size());
        resp->setBody(errorBody);
        return false;
    }
    
    if (body.empty() || (contentType != "application/json" && !contentType.empty())) {
        json errorResp;
        errorResp["success"] = false;
        errorResp["error"] = "Invalid request: Content-Type must be application/json";
        std::string errorBody = errorResp.dump(4);
        
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k400BadRequest, "Bad Request");
        resp->setCloseConnection(false);
        resp->setContentType("application/json");
        resp->setContentLength(errorBody.size());
        resp->setBody(errorBody);
        return false;
    }
    
    try {
        outJson = json::parse(body);
        return true;
    } catch (const std::exception& e) {
        // JSON解析失败时返回400错误
        json errorResp;
        errorResp["success"] = false;
        errorResp["error"] = "Invalid JSON format: " + std::string(e.what());
        std::string errorBody = errorResp.dump(4);
        
        resp->setStatusLine(req.getVersion(), http::HttpResponse::k400BadRequest, "Bad Request");
        resp->setCloseConnection(false);
        resp->setContentType("application/json");
        resp->setContentLength(errorBody.size());
        resp->setBody(errorBody);
        return false;
    }
}