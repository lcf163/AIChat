#pragma once

#include <string>
#include <curl/curl.h>
#include <memory>

#include "utils/base64.h"
#include "SpeechService.h"

/**
 * 百度语音服务实现类
 * 实现SpeechService接口，提供百度语音识别和合成功能
 */
class BaiduSpeechService : public SpeechService {
public:
    BaiduSpeechService(const std::string& clientId,
                       const std::string& clientSecret,
                       const std::string& cuid = "");
    
    ~BaiduSpeechService() = default;

    // 实现语音识别接口
    std::string recognize(const std::string& speechData,
                        const std::string& format = "pcm",
                        int rate = 16000,
                        int channel = 1) override;

    // 实现语音合成接口
    std::string synthesize(const std::string& text,
                          const std::string& format = "mp3-16k",
                          const std::string& lang = "zh",
                          int speed = 5,
                          int pitch = 5,
                          int volume = 5) override;

private:
    std::string client_id_;
    std::string client_secret_;
    std::string cuid_;
    std::string token_;

    // 获取访问令牌
    std::string getAccessToken();
    
    // CURL回调函数
    static size_t onWriteData(void* buffer, size_t size, size_t nmemb, void* userp);
};