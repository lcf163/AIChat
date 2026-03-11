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

    bool isMCPModel = false;
};

class AliyunStrategy : public AIStrategy {
public:
    AliyunStrategy();
    
    std::string getApiUrl() const override;
    std::string getApiKey() const override;
    std::string getModel() const override;

    json buildRequest(const std::vector<std::pair<std::string, long long>>& messages) const override;
    std::string parseResponse(const json& response) const override;

private:
    std::string apiKey_;
    std::string apiUrl_;
    std::string modelName_;
};

class DouBaoStrategy : public AIStrategy {
public:
    DouBaoStrategy();
    
    std::string getApiUrl() const override;
    std::string getApiKey() const override;
    std::string getModel() const override;

    json buildRequest(const std::vector<std::pair<std::string, long long>>& messages) const override;
    std::string parseResponse(const json& response) const override;

private:
    std::string apiKey_;
    std::string apiUrl_;
    std::string modelName_;
};

class AliyunRAGStrategy : public AIStrategy {
public:
    AliyunRAGStrategy();
    
    std::string getApiUrl() const override;
    std::string getApiKey() const override;
    std::string getModel() const override;

    json buildRequest(const std::vector<std::pair<std::string, long long>>& messages) const override;
    std::string parseResponse(const json& response) const override;

private:
    std::string apiKey_;
    std::string knowledgeBaseId_;
    std::string apiUrlPrefix_;
    std::string apiUrlSuffix_;
};

class AliyunMcpStrategy : public AIStrategy {
public:
    AliyunMcpStrategy();
    
    std::string getApiUrl() const override;
    std::string getApiKey() const override;
    std::string getModel() const override;

    json buildRequest(const std::vector<std::pair<std::string, long long>>& messages) const override;
    std::string parseResponse(const json& response) const override;

private:
    std::string apiKey_;
    std::string apiUrl_;
    std::string modelName_;
};