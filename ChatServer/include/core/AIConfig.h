#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <regex>
#include <fstream>
#include <sstream>
#include <iostream>
#include <memory>

#include "utils/JsonUtil.h"  
#include "AIToolRegistry.h"

// 添加日志级别枚举
enum class LogLevel {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};

// 添加语音服务提供商枚举
enum class SpeechServiceProvider {
    BAIDU,
    UNKNOWN
};

// 添加限制配置结构体
struct LimitsConfig {
    int maxActiveSessions;
    int maxHistoryRounds;
    int maxTokensPerMessage;  // 每条消息的最大token数
};

struct AITool {
    std::string name;
    std::unordered_map<std::string, std::string> params;
    std::string desc;
};

struct AIToolCall {
    std::string toolName;
    json args;
    bool isToolCall = false;
};

// 数据库配置结构
struct DatabaseConfig {
    std::string host;
    std::string user;
    std::string password;
    std::string database;
    int poolSize;
};

// 日志配置结构
struct LogConfig {
    LogLevel level;
};

// RabbitMQ配置结构
struct RabbitMQConfig {
    std::string host;
    int port;
    std::string username;
    std::string password;
    std::string vhost;
    std::string queueName;
    int threadNum;
};

// API密钥配置结构
struct ApiKeysConfig {
    std::string dashscopeApiKey;
    std::string knowledgeBaseId;
    std::string baiduClientId;
    std::string baiduClientSecret;
    std::string doubaoApiKey;
    std::string doubaoModelId;
};

// 模型配置结构
struct ModelConfig {
    struct AliyunConfig {
        std::string apiUrl;
        std::string modelName;
    };
    
    struct DoubaoConfig {
        std::string apiUrl;
        std::string modelName;
    };
    
    struct AliyunRagConfig {
        std::string apiUrlPrefix;
        std::string apiUrlSuffix;
    };
    
    struct AliyunMcpConfig {
        std::string apiUrl;
        std::string modelName;
    };
    
    AliyunConfig aliyun;
    DoubaoConfig doubao;
    AliyunRagConfig aliyunRag;
    AliyunMcpConfig aliyunMcp;
};

class AIConfig {
public:
    static AIConfig& getInstance();
    
    bool loadFromFile(const std::string& path);
    std::string buildPrompt(const std::string& userInput) const;
    AIToolCall parseAIResponse(const std::string& response) const;
    std::string buildToolResultPrompt(
        const std::string& userInput,
        const std::string& toolName,
        const json& toolArgs,
        const json& toolResult) const;

    // 获取配置项
    const DatabaseConfig& getDatabaseConfig() const { return dbConfig_; }
    const LogConfig& getLogConfig() const { return logConfig_; }
    const RabbitMQConfig& getRabbitMQConfig() const { return mqConfig_; }
    const ApiKeysConfig& getApiKeysConfig() const { return apiKeysConfig_; }
    const ModelConfig& getModelConfig() const { return modelConfig_; }
    const LimitsConfig& getLimitsConfig() const { return limitsConfig_; }
    SpeechServiceProvider getSpeechServiceProvider() const { return speechServiceProvider_; }
    const AIToolRegistry& getToolRegistry() const { return toolRegistry_; }

private:
    AIConfig(); // 私有构造函数
    ~AIConfig() = default;
    AIConfig(const AIConfig&) = delete; // 禁止拷贝构造
    AIConfig& operator=(const AIConfig&) = delete; // 禁止赋值操作
    
    std::string promptTemplate_;
    std::vector<AITool> tools_;
    mutable AIToolRegistry toolRegistry_; // 用于获取工具列表信息
    bool isLoaded_;

    // 配置项
    DatabaseConfig dbConfig_;
    LogConfig logConfig_;
    RabbitMQConfig mqConfig_;
    ApiKeysConfig apiKeysConfig_;
    ModelConfig modelConfig_;
    LimitsConfig limitsConfig_;
    SpeechServiceProvider speechServiceProvider_;

    std::string buildToolList() const;
};