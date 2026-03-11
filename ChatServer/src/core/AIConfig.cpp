#include <muduo/base/Logging.h>

#include "core/AIConfig.h"

AIConfig::AIConfig() : isLoaded_(false) {
    // 设置默认日志级别为WARN
    logConfig_.level = LogLevel::WARN;
    
    // 设置默认限制配置
    limitsConfig_.maxHistoryRounds = 10;
    limitsConfig_.maxActiveSessions = 1000;
    limitsConfig_.maxTokensPerMessage = 500;  // 默认每条消息最大500个token
    
    // 工具已经在AIToolRegistry构造函数中注册
}

AIConfig& AIConfig::getInstance() {
    static AIConfig instance;
    return instance;
}

bool AIConfig::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        LOG_ERROR << "Failed to open config file: " << path;
        return false;
    }

    try {
        json config;
        file >> config;
        file.close();

        // 加载提示模板（强制从配置文件读取）
        if (config.contains("prompt_template") && config["prompt_template"].is_string()) {
            promptTemplate_ = config["prompt_template"];
        } else {
            LOG_ERROR << "Error: prompt_template not found in config file!";
            return false;
        }

        // 加载数据库配置
        if (config.contains("database")) {
            auto dbConfig = config["database"];
            if (dbConfig.contains("host")) dbConfig_.host = dbConfig["host"];
            if (dbConfig.contains("user")) dbConfig_.user = dbConfig["user"];
            if (dbConfig.contains("password")) dbConfig_.password = dbConfig["password"];
            if (dbConfig.contains("database")) dbConfig_.database = dbConfig["database"];
            if (dbConfig.contains("pool_size")) dbConfig_.poolSize = dbConfig["pool_size"];
        }

        // 加载RabbitMQ配置
        if (config.contains("rabbitmq")) {
            auto mqConfig = config["rabbitmq"];
            if (mqConfig.contains("host")) mqConfig_.host = mqConfig["host"];
            if (mqConfig.contains("port")) mqConfig_.port = mqConfig["port"];
            if (mqConfig.contains("username")) mqConfig_.username = mqConfig["username"];
            if (mqConfig.contains("password")) mqConfig_.password = mqConfig["password"];
            if (mqConfig.contains("vhost")) mqConfig_.vhost = mqConfig["vhost"];
            if (mqConfig.contains("queue_name")) mqConfig_.queueName = mqConfig["queue_name"];
            if (mqConfig.contains("thread_num")) mqConfig_.threadNum = mqConfig["thread_num"];
        }

        // 加载API密钥配置
        if (config.contains("api_keys")) {
            auto apiKeysConfig = config["api_keys"];
            if (apiKeysConfig.contains("dashscope_api_key")) {
                apiKeysConfig_.dashscopeApiKey = apiKeysConfig["dashscope_api_key"];
            }
            if (apiKeysConfig.contains("knowledge_base_id")) {
                apiKeysConfig_.knowledgeBaseId = apiKeysConfig["knowledge_base_id"];
            }
            if (apiKeysConfig.contains("baidu_client_id")) {
                apiKeysConfig_.baiduClientId = apiKeysConfig["baidu_client_id"];
            }
            if (apiKeysConfig.contains("baidu_client_secret")) {
                apiKeysConfig_.baiduClientSecret = apiKeysConfig["baidu_client_secret"];
            }
            if (apiKeysConfig.contains("doubao_api_key")) {
                apiKeysConfig_.doubaoApiKey = apiKeysConfig["doubao_api_key"];
            }
        }

        // 加载模型配置
        if (config.contains("models")) {
            auto modelsConfig = config["models"];
            
            // 加载阿里云模型配置
            if (modelsConfig.contains("aliyun")) {
                auto aliyunConfig = modelsConfig["aliyun"];
                if (aliyunConfig.contains("api_url")) {
                    modelConfig_.aliyun.apiUrl = aliyunConfig["api_url"];
                }
                if (aliyunConfig.contains("model_name")) {
                    modelConfig_.aliyun.modelName = aliyunConfig["model_name"];
                }
            }
            
            // 加载豆包模型配置
            if (modelsConfig.contains("doubao")) {
                auto doubaoConfig = modelsConfig["doubao"];
                if (doubaoConfig.contains("api_url")) {
                    modelConfig_.doubao.apiUrl = doubaoConfig["api_url"];
                }
                if (doubaoConfig.contains("model_name")) {
                    modelConfig_.doubao.modelName = doubaoConfig["model_name"];
                }
            }
            
            // 加载阿里云RAG配置
            if (modelsConfig.contains("aliyun_rag")) {
                auto aliyunRagConfig = modelsConfig["aliyun_rag"];
                if (aliyunRagConfig.contains("api_url_prefix")) {
                    modelConfig_.aliyunRag.apiUrlPrefix = aliyunRagConfig["api_url_prefix"];
                }
                if (aliyunRagConfig.contains("api_url_suffix")) {
                    modelConfig_.aliyunRag.apiUrlSuffix = aliyunRagConfig["api_url_suffix"];
                }
            }
            

        }

        // 加载语音服务提供商配置
        if (config.contains("speech_service")) {
            auto speechServiceConfig = config["speech_service"];
            if (speechServiceConfig.contains("provider")) {
                std::string provider = speechServiceConfig["provider"];
                std::transform(provider.begin(), provider.end(), provider.begin(), ::tolower);
                
                if (provider == "baidu") {
                    speechServiceProvider_ = SpeechServiceProvider::BAIDU;
                } else {
                    speechServiceProvider_ = SpeechServiceProvider::UNKNOWN;
                }
            }
        }
        
        // 加载限制配置
        if (config.contains("limits")) {
            auto limits = config["limits"];
            if (limits.contains("max_history_rounds") && limits["max_history_rounds"].is_number_integer()) {
                limitsConfig_.maxHistoryRounds = limits["max_history_rounds"];
            }
            if (limits.contains("max_active_sessions") && limits["max_active_sessions"].is_number_integer()) {
                limitsConfig_.maxActiveSessions = limits["max_active_sessions"];
            }
            if (limits.contains("max_tokens_per_message") && limits["max_tokens_per_message"].is_number_integer()) {
                limitsConfig_.maxTokensPerMessage = limits["max_tokens_per_message"];
            }
        }

        // 加载日志配置
        if (config.contains("log")) {
            auto logConfig = config["log"];
            if (logConfig.contains("level")) {
                std::string levelStr = logConfig["level"];
                std::transform(levelStr.begin(), levelStr.end(), levelStr.begin(), ::toupper);
                
                if (levelStr == "TRACE") {
                    logConfig_.level = LogLevel::TRACE;
                } else if (levelStr == "DEBUG") {
                    logConfig_.level = LogLevel::DEBUG;
                } else if (levelStr == "INFO") {
                    logConfig_.level = LogLevel::INFO;
                } else if (levelStr == "WARN") {
                    logConfig_.level = LogLevel::WARN;
                } else if (levelStr == "ERROR") {
                    logConfig_.level = LogLevel::ERROR;
                } else if (levelStr == "FATAL") {
                    logConfig_.level = LogLevel::FATAL;
                } else {
                    // 默认使用WARN级别
                    logConfig_.level = LogLevel::WARN;
                }
            }
        }

        isLoaded_ = true;
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR << "Error parsing config file: " << e.what();
        return false;
    }
}

std::string AIConfig::buildToolList() const {
    // 优先使用工具注册中心的工具列表
    std::string registryTools = toolRegistry_.getToolListDescription();
    if (!registryTools.empty()) {
        return registryTools;
    }
    
    // 如果注册中心没有工具，则使用配置文件中的工具列表
    std::ostringstream oss;
    for (const auto& t : tools_) {
        oss << t.name << "(";
        bool first = true;
        for (const auto& [key, val] : t.params) {
            if (!first) oss << ", ";
            oss << key;
            first = false;
        }
        oss << ") -> " << t.desc << "\n";
    }
    return oss.str();
}

std::string AIConfig::buildPrompt(const std::string& userInput) const {
    if (!isLoaded_) {
        return ""; // 配置未加载
    }
    
    // 检查promptTemplate_是否为空
    if (promptTemplate_.empty()) {
        LOG_ERROR << "Error: prompt_template is empty!";
        return "";
    }
    
    std::string result = promptTemplate_;
    result = std::regex_replace(result, std::regex("\\{user_input\\}"), userInput);
    result = std::regex_replace(result, std::regex("\\{tool_list\\}"), buildToolList());
    return result;
}

AIToolCall AIConfig::parseAIResponse(const std::string& response) const {
    AIToolCall result;
    try {
        json j = json::parse(response);

        if (j.contains("tool") && j["tool"].is_string()) {
            result.toolName = j["tool"].get<std::string>();
            if (j.contains("args") && j["args"].is_object()) {
                result.args = j["args"];
            }
            result.isToolCall = true;
        }
    }
    catch (...) {
        result.isToolCall = false;
    }
    return result;
}

std::string AIConfig::buildToolResultPrompt(
    const std::string& userInput,
    const std::string& toolName,
    const json& toolArgs,
    const json& toolResult) const
{
    std::ostringstream oss;
    oss << "下面是用户说的话：" << userInput << "\n"
        << "我刚才调用了工具 [" << toolName << "] ，参数为："
        << toolArgs.dump() << "\n"
        << "工具返回的结果如下：\n" << toolResult.dump(4) << "\n"
        << "请根据以上信息，用自然语言回答用户。";
    return oss.str();
}