#include "core/AIStrategy.h"
#include "core/AIFactory.h"

namespace {
    // 公共的滑动窗口处理函数
    std::pair<size_t, size_t> applySlidingWindow(const std::vector<std::pair<std::string, long long>>& messages) {
        // 从配置文件中获取最大历史轮次
        const auto& limitsConfig = AIConfig::getInstance().getLimitsConfig();
        const size_t MAX_HISTORY_ROUNDS = limitsConfig.maxHistoryRounds;
        const size_t WINDOW_SIZE = MAX_HISTORY_ROUNDS * 2;
        
        size_t total_msgs = messages.size();
        size_t start_index = 0;
        
        // 如果消息总数超过窗口大小，计算截断起始点
        if (total_msgs > WINDOW_SIZE) {
            start_index = total_msgs - WINDOW_SIZE;
            
            // 【关键修正】确保 start_index 指向的是 User 消息 (偶数索引)
            // 这样可以保证 role 赋值逻辑 (i%2) 的正确性，避免 role 错位
            if (start_index % 2 != 0) {
                start_index++; 
            }
        }
        
        return std::make_pair(start_index, total_msgs);
    }
    
    // 公共的消息构建函数
    void buildMessages(json& msgArray, const std::vector<std::pair<std::string, long long>>& messages, 
                       size_t start_index, size_t total_msgs) {
        // 构建上下文
        for (size_t i = start_index; i < total_msgs; ++i) {
            json msg;
            // 基于 vector 绝对索引判断角色：偶数=User, 奇数=Assistant
            // 前提是 loadSessionOnDemand 必须保证加载的第一条是 User 消息
            if (i % 2 == 0) {
                msg["role"] = "user";
            }
            else {
                msg["role"] = "assistant";
            }
            msg["content"] = messages[i].first;
            msgArray.push_back(msg);
        }
    }
}

// AliyunStrategy implementation
AliyunStrategy::AliyunStrategy() {
    const auto& apiKeys = AIConfig::getInstance().getApiKeysConfig();
    const auto& modelConfig = AIConfig::getInstance().getModelConfig();
    
    if (!apiKeys.dashscopeApiKey.empty()) {
        apiKey_ = apiKeys.dashscopeApiKey;
    } else {
        const char* key = std::getenv("DASHSCOPE_API_KEY");
        if (!key) throw std::runtime_error("Aliyun API Key not found!");
        apiKey_ = std::string(key);
    }
    
    apiUrl_ = modelConfig.aliyun.apiUrl;
    modelName_ = modelConfig.aliyun.modelName;
    isMCPModel = false;
}

std::string AliyunStrategy::getApiUrl() const {
    return apiUrl_;
}

std::string AliyunStrategy::getApiKey()const {
    return apiKey_;
}

std::string AliyunStrategy::getModel() const {
    return modelName_;
}

json AliyunStrategy::buildRequest(const std::vector<std::pair<std::string, long long>>& messages) const {
    json payload;
    payload["model"] = getModel();
    json msgArray = json::array();

    // 应用滑动窗口逻辑
    auto [start_index, total_msgs] = applySlidingWindow(messages);
    
    // 构建消息数组
    buildMessages(msgArray, messages, start_index, total_msgs);

    // 阿里云模型特定参数：流式输出增强
    if (!payload.contains("parameters")) {
        payload["parameters"] = json::object();
    }
    payload["parameters"]["incremental_output"] = true;

    payload["messages"] = msgArray;
    return payload;
}

std::string AliyunStrategy::parseResponse(const json& response) const {
    if (response.contains("choices") && !response["choices"].empty()) {
        return response["choices"][0]["message"]["content"];
    }
    return {};
}

// DouBaoStrategy implementation
DouBaoStrategy::DouBaoStrategy() {
    const auto& apiKeys = AIConfig::getInstance().getApiKeysConfig();
    const auto& modelConfig = AIConfig::getInstance().getModelConfig();
    
    if (!apiKeys.doubaoApiKey.empty()) {
        apiKey_ = apiKeys.doubaoApiKey;
    } else {
        const char* key = std::getenv("DOUBAO_API_KEY");
        if (!key) throw std::runtime_error("DOUBAO API Key not found!");
        apiKey_ = std::string(key);
    }
    
    // 如果配置中有指定的模型ID，则使用配置中的，否则使用默认的
    if (!apiKeys.doubaoModelId.empty()) {
        modelName_ = apiKeys.doubaoModelId;
    } else {
        modelName_ = modelConfig.doubao.modelName;
    }
    
    apiUrl_ = modelConfig.doubao.apiUrl;
    isMCPModel = false;
}

std::string DouBaoStrategy::getApiUrl()const {
    return apiUrl_;
}

std::string DouBaoStrategy::getApiKey()const {
    return apiKey_;
}

std::string DouBaoStrategy::getModel() const {
    return modelName_;
}

json DouBaoStrategy::buildRequest(const std::vector<std::pair<std::string, long long>>& messages) const {
    json payload;
    payload["model"] = getModel();
    json msgArray = json::array();

    // 应用滑动窗口逻辑
    auto [start_index, total_msgs] = applySlidingWindow(messages);
    
    // 构建消息数组
    buildMessages(msgArray, messages, start_index, total_msgs);

    payload["messages"] = msgArray;
    return payload;
}

std::string DouBaoStrategy::parseResponse(const json& response) const {
    if (response.contains("choices") && !response["choices"].empty()) {
        return response["choices"][0]["message"]["content"];
    }
    return {};
}

// AliyunRAGStrategy implementation
AliyunRAGStrategy::AliyunRAGStrategy() {
    const auto& apiKeys = AIConfig::getInstance().getApiKeysConfig();
    const auto& modelConfig = AIConfig::getInstance().getModelConfig();
    
    if (!apiKeys.dashscopeApiKey.empty()) {
        apiKey_ = apiKeys.dashscopeApiKey;
    } else {
        const char* key = std::getenv("DASHSCOPE_API_KEY");
        if (!key) throw std::runtime_error("Aliyun API Key not found!");
        apiKey_ = std::string(key);
    }
    
    if (!apiKeys.knowledgeBaseId.empty()) {
        knowledgeBaseId_ = apiKeys.knowledgeBaseId;
    } else {
        const char* key = std::getenv("Knowledge_Base_ID");
        if (!key) throw std::runtime_error("Knowledge_Base_ID not found!");
        knowledgeBaseId_ = std::string(key);
    }
    
    apiUrlPrefix_ = modelConfig.aliyunRag.apiUrlPrefix;
    apiUrlSuffix_ = modelConfig.aliyunRag.apiUrlSuffix;
    isMCPModel = false;
}

std::string AliyunRAGStrategy::getApiUrl() const {
    return apiUrlPrefix_ + knowledgeBaseId_ + apiUrlSuffix_;
}

std::string AliyunRAGStrategy::getApiKey()const {
    return apiKey_;
}

std::string AliyunRAGStrategy::getModel() const {
    return "";
}

json AliyunRAGStrategy::buildRequest(const std::vector<std::pair<std::string, long long>>& messages) const {
    json payload;
    json msgArray = json::array();
    
    // 应用滑动窗口逻辑
    auto [start_index, total_msgs] = applySlidingWindow(messages);
    
    // 构建消息数组
    buildMessages(msgArray, messages, start_index, total_msgs);
    
    payload["input"]["messages"] = msgArray;
    payload["parameters"] = json::object(); 
    return payload;
}

std::string AliyunRAGStrategy::parseResponse(const json& response) const {
    if (response.contains("output") && response["output"].contains("text")) {
        return response["output"]["text"];
    }
    return {};
}

// AliyunMcpStrategy implementation
AliyunMcpStrategy::AliyunMcpStrategy() {
    const auto& apiKeys = AIConfig::getInstance().getApiKeysConfig();
    const auto& modelConfig = AIConfig::getInstance().getModelConfig();
    
    if (!apiKeys.dashscopeApiKey.empty()) {
        apiKey_ = apiKeys.dashscopeApiKey;
    } else {
        const char* key = std::getenv("DASHSCOPE_API_KEY");
        if (!key) throw std::runtime_error("Aliyun API Key not found!");
        apiKey_ = std::string(key);
    }
    
    apiUrl_ = modelConfig.aliyunMcp.apiUrl;
    modelName_ = modelConfig.aliyunMcp.modelName;
    isMCPModel = true;
}

std::string AliyunMcpStrategy::getApiUrl() const {
    return apiUrl_;
}

std::string AliyunMcpStrategy::getApiKey()const {
    return apiKey_;
}

std::string AliyunMcpStrategy::getModel() const {
    return modelName_;
}

json AliyunMcpStrategy::buildRequest(const std::vector<std::pair<std::string, long long>>& messages) const {
    json payload;
    payload["model"] = getModel();
    json msgArray = json::array();

    // 应用滑动窗口逻辑
    auto [start_index, total_msgs] = applySlidingWindow(messages);
    
    // 构建消息数组
    buildMessages(msgArray, messages, start_index, total_msgs);

    payload["messages"] = msgArray;
    return payload;
}

std::string AliyunMcpStrategy::parseResponse(const json& response) const {
    if (response.contains("choices") && !response["choices"].empty()) {
        return response["choices"][0]["message"]["content"];
    }
    return {};
}

static StrategyRegister<AliyunStrategy> regAliyun("1");
static StrategyRegister<DouBaoStrategy> regDoubao("2");
static StrategyRegister<AliyunRAGStrategy> regAliyunRag("3");
static StrategyRegister<AliyunMcpStrategy> regAliyunMcp("4");