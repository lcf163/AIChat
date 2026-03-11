#ifndef PARSEJSONUTIL_H
#define PARSEJSONUTIL_H

#include <nlohmann/json.hpp>
#include <http/HttpRequest.h>
#include <http/HttpResponse.h>

using json = nlohmann::json;

class ParseJsonUtil {
public:
    /**
     * 解析HTTP请求中的JSON数据
     * @param req HTTP请求
     * @param resp HTTP响应
     * @param outJson 解析后的JSON对象
     * @return 是否解析成功
     */
    static bool parseJsonFromBody(const http::HttpRequest& req, http::HttpResponse* resp, json& outJson);
};

#endif // PARSEJSONUTIL_H