#include <chrono>
#include <muduo/base/Logging.h>

#include "utils/MQManager.h"
#include "core/AIHelper.h"

// 简单的 SSE 数据解析器 (提取 content)
std::string parseLLMChunk(const std::string& chunk) {
    std::string content;
    std::stringstream ss(chunk);
    std::string line;
    
    while (std::getline(ss, line)) {
        // 去除行尾可能的 \r
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.empty()) continue;

        // 情况1：标准的 SSE 格式 "data: {...}"
        if (line.find("data: ") == 0) {
            std::string jsonStr = line.substr(6); // 去掉 "data: "
            if (jsonStr == "[DONE]") continue; // 忽略结束标记
            
            try {
                json j = json::parse(jsonStr);
                // 1. 兼容 OpenAI 格式结构
                if (j.contains("choices") && !j["choices"].empty()) {
                    auto& choice = j["choices"][0];
                    if (choice.contains("delta") && choice["delta"].contains("content")) {
                         if (!choice["delta"]["content"].is_null()) {
                            content += choice["delta"]["content"].get<std::string>();
                         }
                    }
                    else if (choice.contains("message") && choice["message"].contains("content")) {
                         if (!choice["message"]["content"].is_null()) {
                            content += choice["message"]["content"].get<std::string>();
                         }
                    }
                }
                // 2. 兼容 阿里百炼原生/RAG 格式结构 (output -> text) - 流式
                else if (j.contains("output") && j["output"].contains("text")) {
                     if (!j["output"]["text"].is_null()) {
                        content += j["output"]["text"].get<std::string>();
                     }
                }
            } catch (...) {
                // 忽略解析错误的行
            }
        } 
        // 情况2：非 SSE 的完整 JSON 响应 (例如 RAG 同步返回)
        else if (line[0] == '{') {
             try {
                json j = json::parse(line);
                if (j.contains("output") && j["output"].contains("text")) {
                    if (!j["output"]["text"].is_null()) {
                        content += j["output"]["text"].get<std::string>();
                    }
                }
             } catch (...) {}
        }
    }
    return content;
}

enum modelType {
    AliType = 1,
    DouType = 2,
    AliRAGType = 3,
    AliMCPType = 4
};

// 构造函数
AIHelper::AIHelper() {
    // 默认使用阿里大模型
    strategy = StrategyFactory::instance().create(std::to_string(AliType));
}

void AIHelper::setStrategy(std::shared_ptr<AIStrategy> strat) {
    strategy = strat;
}

// 根据token数量截断消息
std::string AIHelper::truncateMessageByTokens(const std::string& message) {
    // 获取配置中的最大token数
    const auto& limitsConfig = AIConfig::getInstance().getLimitsConfig();
    int maxTokens = limitsConfig.maxTokensPerMessage;
    
    // 如果maxTokens <= 0，表示不限制
    if (maxTokens <= 0) {
        return message;
    }
    
    // 计算当前消息的token数 (根据中英文字符计算)
    int currentTokens = calculateTokens(message);
    
    // 如果当前token数未超过限制，直接返回原消息
    if (currentTokens <= maxTokens) {
        return message;
    }
    
    // 截断消息以满足token限制
    // 简单按比例截断，实际应用中可能需要更复杂的逻辑
    double ratio = (double)maxTokens / currentTokens;
    size_t newLength = (size_t)(message.length() * ratio);
    
    // 确保至少有一些内容
    if (newLength < 10) {
        newLength = 10;
    }
    
    LOG_INFO << "Truncating message from " << currentTokens << " tokens to limit of " << maxTokens << " tokens";
    
    // 返回截断后的消息，并添加更友好的提示
    return message.substr(0, newLength) + "... [消息过长，已被截断]";
}

int AIHelper::calculateTokens(const std::string& text) {
    int asciiCount = 0;
    int nonAsciiTokens = 0;
    
    for (size_t i = 0; i < text.length();) {
        if ((text[i] & 0x80) != 0 && (text[i] & 0xE0) == 0xE0) {
            // 中文字符 (UTF-8编码，3字节)
            // 中文按1字符≈1 Token计算
            nonAsciiTokens += 1;
            i += 3;
        } else {
            // 英文字符或其他ASCII字符
            // 英文按4字符≈1 Token计算
            asciiCount++;
            i += 1;
        }
    }
    
    // 总token数 = 中文字符数 + (英文字符数+3)/4 (向上取整)
    return nonAsciiTokens + (asciiCount + 3) / 4;
}

// 添加一条用户消息
void AIHelper::addMessage(int userId, const std::string& userName, bool is_user,const std::string& userInput, std::string sessionId) {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    
    // 对用户输入进行token限制检查和截断
    std::string processedInput = userInput;
    if (is_user) {  // 只对用户消息进行截断处理
        processedInput = truncateMessageByTokens(userInput);
    }
    
    messages.push_back({ processedInput, ms });
    //消息队列异步入库
    pushMessageToMysql(userId, userName, is_user, processedInput, ms, sessionId);
}

// 发送聊天消息
std::string AIHelper::chat(int userId,std::string userName, std::string sessionId, std::string userQuestion, std::string modelType) {
    return chatImpl(userId, userName, sessionId, userQuestion, modelType, nullptr);
}

// 异步发送聊天消息
std::future<std::string> AIHelper::chatAsync(std::shared_ptr<ThreadPool> pool, int userId, std::string userName, std::string sessionId, std::string userQuestion, std::string modelType, StreamCallback callback) {
    // 使用线程池执行任务
    return pool->enqueue([=]() {
        return chatImpl(userId, userName, sessionId, userQuestion, modelType, callback);
    });
}

// 实际执行聊天逻辑的方法
std::string AIHelper::chatImpl(int userId, std::string userName, std::string sessionId, std::string userQuestion, std::string modelType, StreamCallback callback) {
    // 设置策略
    setStrategy(StrategyFactory::instance().create(modelType));
    
    // 判断是否为 RAG 模型
    bool isRAG = (modelType == std::to_string(AliRAGType));

    if (!strategy->isMCPModel) {
        addMessage(userId, userName, true, userQuestion, sessionId);
        json payload = strategy->buildRequest(this->messages);
        
        std::string fullResponse;
         
        // 只有当有 callback 且 不是 RAG 模型时，才启用流式传输。
        // 如果是 RAG 模型，即使有 callback，也强制进入 else 分支（非流式）。
        if (callback && !isRAG) {
            // 创建一个包装回调，用于累积完整响应
            StreamCallback wrappedCallback = [callback, &fullResponse](const std::string& delta) {
                fullResponse += delta;
                callback(delta);
            };
            
            payload["stream"] = true; // 强制流式

            // // 针对普通阿里模型可能需要的参数
            // if (payload.contains("input")) {
            //     if (!payload.contains("parameters")) {
            //         payload["parameters"] = json::object();
            //     }
            //     // 普通应用开启流式时推荐开启增量输出，防止重复
            //     payload["parameters"]["incremental_output"] = true;
            // }

            executeCurl(payload, wrappedCallback);
            
            // 保存AI助手的完整回复到数据库
            addMessage(userId, userName, false, fullResponse, sessionId);
            return fullResponse; 

        } else {
            // ==== 非流式模式 ====
            
            // 确保不包含 stream 参数 (防止污染)
            if (payload.contains("stream")) {
                payload.erase("stream");
            }

            // 执行同步请求，获取完整 JSON
            // 注意：这里传入 nullptr 作为 callback，告诉 executeCurl 不要走流式处理
            json response = executeCurl(payload, nullptr);
            std::string answer;

            // 解析响应：优先尝试解析阿里 RAG 的 output.text 格式
            if (response.contains("output") && response["output"].contains("text")) {
                if (!response["output"]["text"].is_null()) {
                    answer = response["output"]["text"].get<std::string>();
                }
            } else {
                // 回退到 Strategy 的默认解析 (通常是 OpenAI 格式)
                answer = strategy->parseResponse(response);
            }
            
            // 如果是 RAG 模式且有回调（前端在等 SSE），手动把完整结果推回去
            // 这样前端虽然等一会儿，但最终会收到一个 SSE 事件，显示完整内容
            if (callback && !answer.empty()) {
                callback(answer);
            }

            if (answer.empty()) {
                answer = "[Error] 无法解析响应或响应为空";
                LOG_ERROR << "Failed to parse response: " << response.dump();
            }

            addMessage(userId, userName, false, answer, sessionId);
            return answer;
        }
    }
    
    // ======== MCP / Tool Call 逻辑 ========

    AIConfig& config = AIConfig::getInstance();
    std::string tempUserQuestion = config.buildPrompt(userQuestion);
    messages.push_back({ tempUserQuestion, 0 });

    json firstReq = strategy->buildRequest(this->messages);
    json firstResp = executeCurl(firstReq); 
    std::string aiResult = strategy->parseResponse(firstResp);
    messages.pop_back();

    AIToolCall call = config.parseAIResponse(aiResult);

    if (call.isToolCall) {
        std::shared_ptr<Tool> tool = config.getToolRegistry().getTool(call.toolName);
        if (tool) {
            json toolParams = tool->getParameters();
            bool valid = true;
            std::string errorMsg;
            
            if (toolParams.contains("required") && toolParams["required"].is_array()) {
                for (const auto& requiredParam : toolParams["required"]) {
                    if (requiredParam.is_string()) {
                        std::string paramName = requiredParam.get<std::string>();
                        if (!call.args.contains(paramName) || 
                            call.args[paramName].is_null() || 
                            (call.args[paramName].is_string() && call.args[paramName].get<std::string>().empty())) {
                            valid = false;
                            errorMsg = "缺少必要参数: " + paramName;
                            break;
                        }
                    }
                }
            }
            
            if (valid) {
                try {
                    json toolResult = tool->execute(call.args);
                    messages.push_back({ "[工具调用结果]\n" + toolResult.dump(4), 0 });
                } catch (const std::exception& e) {
                    messages.push_back({ "[工具执行错误]\n" + std::string(e.what()), 0 });
                }
            } else {
                messages.push_back({ "[参数验证失败]\n" + errorMsg, 0 });
            }
        } else {
            messages.push_back({ "[工具调用失败]\n未找到工具: " + call.toolName, 0 });
        }
    } else {
        messages.push_back({ aiResult, 0 });
    }

    // 第二步请求
    json secondReq = strategy->buildRequest(this->messages);
    json secondResp = executeCurl(secondReq);
    std::string finalAnswer = strategy->parseResponse(secondResp);
    messages.push_back({ finalAnswer, 0 });
    
    addMessage(userId, userName, false, finalAnswer, sessionId);
    
    // 如果有回调，发送最终结果
    if (callback) {
        callback(finalAnswer);
    }

    return finalAnswer;
}

// 发送自定义请求体
json AIHelper::request(const json& payload) {
    return executeCurl(payload);
}


// 执行 curl 请求
json AIHelper::executeCurl(const json& payload, StreamCallback callback) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize curl");
    }

    LOG_INFO << "AI API invoke " << strategy->getApiUrl().c_str() << ' ' << strategy->getApiKey();

    std::string readBuffer;
    struct curl_slist* headers = nullptr;
    std::string authHeader = "Authorization: Bearer " + strategy->getApiKey();

    headers = curl_slist_append(headers, authHeader.c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    
    // 仅在真正启用流式回调时添加 Accept 头
    if (callback) {
        headers = curl_slist_append(headers, "Accept: text/event-stream");
    }

    std::string payloadStr = payload.dump();

    CurlContext ctx;
    ctx.buffer = &readBuffer;
    ctx.callback = callback; // 如果是 RAG 同步调用，这里传入的是 nullptr

    curl_easy_setopt(curl, CURLOPT_URL, strategy->getApiUrl().c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payloadStr.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ctx);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L); 

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::string err = "curl_easy_perform() failed: " + std::string(curl_easy_strerror(res));
        LOG_ERROR << err;
        if (!callback) throw std::runtime_error(err);
        return json::object();
    }

    // 如果是流式调用，返回空json
    if (callback) {
        return json::object(); 
    }

    // 非流式调用，解析完整 JSON 并返回
    try {
        if (readBuffer.empty()) return json::object();
        return json::parse(readBuffer);
    }
    catch (const std::exception& e) {
        LOG_ERROR << "Failed to parse JSON response: " << e.what() << " | Body: " << readBuffer;
        throw std::runtime_error("Failed to parse JSON response");
    }
}

// curl 回调函数，把返回的数据写到 string buffer
size_t AIHelper::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    auto* ctx = static_cast<CurlContext*>(userp);
    
    std::string rawData(static_cast<char*>(contents), totalSize);
    ctx->buffer->append(rawData);
    
    LOG_INFO << "Raw data received (" << totalSize << " bytes): " << rawData;
    
    // 实时解析并回调
    if (ctx->callback) {
        std::string deltaText = parseLLMChunk(rawData);
        if (!deltaText.empty()) {
            ctx->callback(deltaText);
        }
        
        // 检查是否是流式结束标记
        if (rawData.find("[DONE]") != std::string::npos) {
            LOG_INFO << "Streaming AI response completed. ";
        }
    } else {
        // 输出非流式调用结束
        LOG_INFO << "Full AI response completed. ";
    }
    return totalSize;
}

void AIHelper::pushMessageToMysql(int userId, const std::string& userName, bool is_user, const std::string& userInput, long long ms, std::string sessionId) {
    // 构造JSON格式的消息
    json messageJson;
    messageJson["sql"] = "INSERT INTO chat_message (user_id, username, session_id, is_user, content, ts) VALUES (?, ?, ?, ?, ?, ?)";
    messageJson["params"] = json::array();
    messageJson["params"].push_back(std::to_string(userId));
    messageJson["params"].push_back(userName);
    messageJson["params"].push_back(sessionId);
    messageJson["params"].push_back(std::to_string(is_user ? 1 : 0));
    messageJson["params"].push_back(userInput);  // userInput可能包含特殊字符，但通过参数化查询处理
    messageJson["params"].push_back(std::to_string(ms));
    
    // 将JSON序列化为字符串发送到消息队列
    std::string message = messageJson.dump();
    
    // 消息队列异步执行mysql操作，用于流量削峰与解耦逻辑
    MQManager::instance().publish("sql_queue", message);
}