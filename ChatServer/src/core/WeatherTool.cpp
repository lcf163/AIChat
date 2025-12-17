#include "core/WeatherTool.h"
#include <curl/curl.h>
#include <thread>
#include <chrono>

std::string WeatherTool::getName() const {
    return "get_weather";
}

std::string WeatherTool::getDescription() const {
    return "获取指定城市的天气信息";
}

json WeatherTool::getParameters() const {
    json params;
    params["type"] = "object";
    params["properties"]["city"] = {
        {"type", "string"},
        {"description", "城市名称"}
    };
    params["required"] = {"city"};
    return params;
}

size_t WeatherTool::WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

json WeatherTool::execute(const json& args) const {
    if (!args.contains("city")) {
        return json{ {"error", "Missing parameter: city"} };
    }

    std::string city = args["city"].get<std::string>();
    std::string encodedCity;
    char* encoded = curl_easy_escape(nullptr, city.c_str(), city.length());
    
    if (encoded) {
        encodedCity = encoded;
        curl_free(encoded);
    }
    else {
        return json{ {"error", "URL encode failed"} };
    }

    std::string url = "https://wttr.in/" + encodedCity + "?format=3&lang=zh";
    CURL* curl = curl_easy_init();
    std::string response;

    if (!curl) {
        return json{ {"error", "网络请求失败，请稍后再试"} };
    }

    // 设置CURL选项
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L); // 10秒超时
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");

    // 尝试发送请求，最多重试3次
    CURLcode res;
    int retryCount = 0;
    const int maxRetries = 3;
    
    do {
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            retryCount++;
            if (retryCount <= maxRetries) {
                // 等待一段时间再重试
                std::this_thread::sleep_for(std::chrono::milliseconds(500 * retryCount));
            }
        }
    } while (res != CURLE_OK && retryCount <= maxRetries);

    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        return json{ {"error", "网络请求失败，请稍后再试"} };
    }
    
    // 检查响应是否为空或者包含错误信息
    if (response.empty()) {
        return json{ {"error", "未能获取到天气信息，请稍后再试"} };
    }
    
    // 检查响应是否包含错误信息
    if (response.find("Unknown location") != std::string::npos) {
        return json{ {"error", "未知的城市名称，请检查输入是否正确"} };
    }
    
    return json{ {"city", city}, {"weather", response} };
}