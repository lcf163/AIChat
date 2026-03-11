#pragma once

#include <string>
#include <vector>
#include <utility>
#include <iostream>
#include <sstream>
#include <memory>

#include "utils/JsonUtil.h"

#include "core/AIConfig.h"

class AIStrategy {
public:
    virtual ~AIStrategy() = default;

    virtual std::string getApiUrl() const = 0;
    
    virtual std::string getApiKey() const = 0;

    virtual std::string getModel() const = 0;

    virtual json buildRequest(const std::vector<std::pair<std::string, long long>>& messages) const = 0;

    virtual std::string parseResponse(const json& response) const = 0;

    // 验证配置有效性
    virtual bool validateConfig() const {
        return !getApiUrl().empty() && !getApiKey().empty();
    }

    // 获取模型信息
    virtual std::string getModelInfo() const {
        return "Model: " + getModel() + ", API URL: " + getApiUrl();
    }

    // 检查是否支持流式输出
    virtual bool supportStreamOutput() const {
        return true;
    }

    // 检查是否支持工具调用 (MCP - Model Context Protocol)
    virtual bool supportToolCall() const {
        return false;
    }
};

// 基础模型策略类，封装公共逻辑
class BaseModelStrategy : public AIStrategy {
public:
    BaseModelStrategy() = default;
    ~BaseModelStrategy() override = default;

    // 应用滑动窗口逻辑
    std::pair<int, int> applySlidingWindow(const std::vector<std::pair<std::string, long long>>& messages) const;

    // 构建消息数组
    void buildMessages(json& msgArray, const std::vector<std::pair<std::string, long long>>& messages, int start_index, int total_msgs) const;

protected:
    // 从环境变量或配置中获取 API 密钥
    std::string getApiKeyFromConfig(const std::string& configKey, const std::string& envKey) const;

    std::string apiKey_;
    std::string apiUrl_;
    std::string modelName_;
};

class AliyunStrategy : public BaseModelStrategy {
public:
    AliyunStrategy();
    
    std::string getApiUrl() const override;
    std::string getApiKey() const override;
    std::string getModel() const override;

    json buildRequest(const std::vector<std::pair<std::string, long long>>& messages) const override;
    std::string parseResponse(const json& response) const override;
    
    bool supportToolCall() const override {
        return true;
    }
};

class DouBaoStrategy : public BaseModelStrategy {
public:
    DouBaoStrategy();
    
    std::string getApiUrl() const override;
    std::string getApiKey() const override;
    std::string getModel() const override;

    json buildRequest(const std::vector<std::pair<std::string, long long>>& messages) const override;
    std::string parseResponse(const json& response) const override;
    
    bool supportToolCall() const override {
        return true;
    }
};

class AliyunRAGStrategy : public BaseModelStrategy {
public:
    AliyunRAGStrategy();
    
    std::string getApiUrl() const override;
    std::string getApiKey() const override;
    std::string getModel() const override;

    json buildRequest(const std::vector<std::pair<std::string, long long>>& messages) const override;
    std::string parseResponse(const json& response) const override;

private:
    std::string knowledgeBaseId_;
    std::string apiUrlPrefix_;
    std::string apiUrlSuffix_;
};