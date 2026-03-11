#pragma once

#include <muduo/net/TcpServer.h>
#include <muduo/net/TcpConnection.h>
#include <functional>
#include <memory>

namespace http
{

class HttpResponse 
{
public:
    // 流式写入回调函数类型
    using StreamWriteCallback = std::function<bool(muduo::net::TcpConnectionPtr conn, HttpResponse* resp)>;
    
    enum HttpStatusCode
    {
        kUnknown,
        k200Ok = 200,
        k204NoContent = 204,
        k301MovedPermanently = 301,
        k400BadRequest = 400,
        k401Unauthorized = 401,
        k403Forbidden = 403,
        k404NotFound = 404,
        k409Conflict = 409,
        k500InternalServerError = 500,
    };

    HttpResponse(bool close = true)
        : statusCode_(kUnknown)
        , closeConnection_(close)
        , isStreaming_(false)
    {}

    void setVersion(std::string version)
    { httpVersion_ = version; }
    void setStatusCode(HttpStatusCode code)
    { statusCode_ = code; }

    HttpStatusCode getStatusCode() const
    { return statusCode_; }

    void setStatusMessage(const std::string message)
    { statusMessage_ = message; }

    void setCloseConnection(bool on)
    { closeConnection_ = on; }

    bool closeConnection() const
    { return closeConnection_; }
    
    void setContentType(const std::string& contentType)
    { addHeader("Content-Type", contentType); }

    void setContentLength(uint64_t length)
    { addHeader("Content-Length", std::to_string(length)); }

    void addHeader(const std::string& key, const std::string& value)
    { headers_[key] = value; }
    
    const std::string& getHeader(const std::string& key) const
    {
        static const std::string empty;
        auto it = headers_.find(key);
        return it != headers_.end() ? it->second : empty;
    }
    
    void setBody(const std::string& body)
    { 
        body_ = body;
        // body_ += "\0";
    }

    // 设置流式响应模式
    void setStreamingMode(bool streaming)
    {
        isStreaming_ = streaming;
    }

    // 设置流式写入回调
    void setStreamWriteCallback(StreamWriteCallback callback)
    {
        streamWriteCallback_ = std::move(callback);
    }

    // 检查是否为流式响应
    bool isStreaming() const
    {
        return isStreaming_;
    }

    // 获取流式写入回调
    const StreamWriteCallback& getStreamWriteCallback() const
    {
        return streamWriteCallback_;
    }

    void setStatusLine(const std::string& version,
                         HttpStatusCode statusCode,
                         const std::string& statusMessage);

    void setErrorHeader(){}

    void appendToBuffer(muduo::net::Buffer* outputBuf) const;
private:
    std::string                        httpVersion_; 
    HttpStatusCode                     statusCode_;
    std::string                        statusMessage_;
    bool                               closeConnection_;
    std::map<std::string, std::string> headers_;
    std::string                        body_;
    bool                               isFile_;
    
    // 流式响应相关字段
    bool                               isStreaming_;
    StreamWriteCallback                streamWriteCallback_;
};

} // namespace http