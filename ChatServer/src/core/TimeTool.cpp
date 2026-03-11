#include "core/TimeTool.h"
#include <ctime>
#include <iomanip>
#include <sstream>

std::string TimeTool::getName() const {
    return "get_time";
}

std::string TimeTool::getDescription() const {
    return "获取当前时间";
}

json TimeTool::getParameters() const {
    json params;
    params["type"] = "object";
    params["properties"] = json::object();
    return params;
}

json TimeTool::execute(const json& args) const {
    (void)args; // 参数未使用
    
    // 获取当前时间
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    // 转换为本地时间
    std::tm* local_time = std::localtime(&time_t);
    
    // 格式化时间
    std::ostringstream oss;
    oss << std::put_time(local_time, "%Y-%m-%d %H:%M:%S");
    
    return json{ {"time", oss.str()} };
}