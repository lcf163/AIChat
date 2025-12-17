#pragma once

#include <string>
#include <vector>
#include <utility>
#include <curl/curl.h>
#include <iostream>
#include <sstream>
#include <functional>
#include <future>

#include "utils/JsonUtil.h"
#include "utils/MysqlUtil.h"
#include "utils/ThreadPool.h"

#include "AIConfig.h"
#include "AIFactory.h"
#include "AIToolRegistry.h"

// 定义回调类型
using StreamCallback = std::function<void(const std::string&)>;

//封装curl访问模型
class AIHelper {
public:
    // 构造函数，初始化API Key
    AIHelper();

    void setStrategy(std::shared_ptr<AIStrategy> strat);

    // 添加一条消息
    void addMessage(int userId, const std::string& userName, bool is_user, const std::string& userInput, std::string sessionId);
    // 发送聊天消息，返回AI的响应内容
    // messages: [{"role":"system","content":"..."}, {"role":"user","content":"..."}]
    std::string chat(int userId, std::string userName, std::string sessionId, std::string userQuestion, std::string modelType);

    // 异步发送聊天消息，支持流式回调
    std::future<std::string> chatAsync(std::shared_ptr<ThreadPool> pool, int userId, std::string userName, std::string sessionId, std::string userQuestion, std::string modelType, StreamCallback callback = nullptr);

    // 发送自定义请求体
    json request(const json& payload);

private:
    // 线程池做异步 mysql 更新操作
    void pushMessageToMysql(int userId, const std::string& userName, bool is_user, const std::string& userInput, long long ms,std::string sessionId);

    // 根据token数量截断消息
    std::string truncateMessageByTokens(const std::string& message);
    
    // 计算文本的token数量
    int calculateTokens(const std::string& text);

    // 执行 curl 请求，返回原始 JSON
    json executeCurl(const json& payload, StreamCallback callback = nullptr);
    // curl 回调函数，把返回的数据写到 string buffer
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

    // 实际执行聊天逻辑的方法
    std::string chatImpl(int userId, std::string userName, std::string sessionId, std::string userQuestion, std::string modelType, StreamCallback callback = nullptr);

private:
    std::shared_ptr<AIStrategy> strategy;

    //一个用户针对一个AIHelper，messages存放用户的历史对话
    //偶数下标代表用户的信息，奇数下标是ai返回的内容
    //后者代表时间戳
    std::vector<std::pair<std::string, long long>> messages;
    
    // 辅助结构体
    struct CurlContext {
        std::string* buffer;
        StreamCallback callback;
    };
};