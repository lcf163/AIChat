#include "handlers/ChatSpeechHandler.h"
#include "utils/JsonUtil.h"
#include "utils/ParseJsonUtil.h"
#include "core/AIConfig.h"
#include "core/AIFactory.h"

void ChatSpeechHandler::handle(const http::HttpRequest& req, http::HttpResponse* resp)
{
    try
    {
        std::string text = "欢迎使用AI智能助手";
        json j;
        if (!ParseJsonUtil::parseJsonFromBody(req, resp, j)) {
            // 错误响应已经在parseJsonFromBody中设置
            return;
        }
        
        if (!j.empty()) {
            if (j.contains("text")) text = j["text"];
        }

        // 获取百度API密钥
        auto& apiKeys = AIConfig::getInstance().getApiKeysConfig();
        std::string clientId, clientSecret;
        
        if (!apiKeys.baiduClientSecret.empty()) {
            clientSecret = apiKeys.baiduClientSecret;
        } else {
            throw std::runtime_error("BAIDU_CLIENT_SECRET not found in config!");
        }
        
        if (!apiKeys.baiduClientId.empty()) {
            clientId = apiKeys.baiduClientId;
        } else {
            throw std::runtime_error("BAIDU_CLIENT_ID not found in config!");
        }
        
        // 通过工厂创建语音服务实例
        auto speechServiceProvider = AIConfig::getInstance().getSpeechServiceProvider();
        std::unique_ptr<SpeechService> speechService = SpeechServiceFactory::createSpeechService(
            speechServiceProvider, clientId, clientSecret);
        
        if (!speechService) {
            throw std::runtime_error("Failed to create speech service instance");
        }
        
        // 获取语音合成后的地址
        std::string speechUrl = speechService->synthesize(text,
                                                         "mp3-16k", 
                                                         "zh", 
                                                         5,  // speed
                                                         5,  // pitch
                                                         5); // volume
                                                         
        // 构造响应JSON，与前端JavaScript代码中期望的字段匹配
        json responseJson;
        responseJson["success"] = true;
        responseJson["url"] = speechUrl;
        responseJson["text"] = text;

        std::string responseBody = responseJson.dump(4);

        server_->packageResp(req.getVersion(), 
                            http::HttpResponse::k200Ok, 
                            "OK", 
                            false, 
                            "application/json", 
                            responseBody.length(), 
                            responseBody, 
                            resp);
    }
    catch (const std::exception& e)
    {
        json errorResp;
        errorResp["success"] = false;
        errorResp["message"] = e.what();
        std::string errorBody = errorResp.dump(4);

        server_->packageResp(req.getVersion(), 
                            http::HttpResponse::k500InternalServerError, 
                            "Internal Server Error", 
                            true, 
                            "application/json", 
                            errorBody.length(), 
                            errorBody, 
                            resp);
    }
}