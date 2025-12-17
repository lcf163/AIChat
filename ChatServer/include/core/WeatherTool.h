#pragma once

#include "Tool.h"
#include <curl/curl.h>

/**
 * @brief 天气查询工具
 */
class WeatherTool : public Tool {
public:
    std::string getName() const override;
    std::string getDescription() const override;
    json getParameters() const override;
    json execute(const json& args) const override;

private:
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output);
};