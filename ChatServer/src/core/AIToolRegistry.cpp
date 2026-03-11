#include <sstream>

#include "core/AIToolRegistry.h"
#include "core/WeatherTool.h"
#include "core/TimeTool.h"

AIToolRegistry::AIToolRegistry() {
    // 注册基于类的工具
    registerTool(std::make_shared<WeatherTool>());
    registerTool(std::make_shared<TimeTool>());
}

// 注册函数式工具
void AIToolRegistry::registerTool(const std::string& name, ToolFunc func) {
    functionTools_[name] = func;
}

// 注册基于Tool类的工具
void AIToolRegistry::registerTool(std::shared_ptr<Tool> tool) {
    std::string name = tool->getName();
    classTools_[name] = tool;
    
    // 同时注册为函数式工具，保持向后兼容
    functionTools_[name] = [tool](const json& args) -> json {
        return tool->execute(args);
    };
}

// 调用工具
json AIToolRegistry::invoke(const std::string& name, const json& args) const {
    auto it = functionTools_.find(name);
    if (it == functionTools_.end()) {
        throw std::runtime_error("Tool not found: " + name);
    }
    return it->second(args);
}

bool AIToolRegistry::hasTool(const std::string& name) const {
    return functionTools_.count(name) > 0;
}

std::string AIToolRegistry::getToolListDescription() const {
    std::ostringstream oss;
    
    // 输出基于类的工具信息
    for (const auto& pair : classTools_) {
        auto tool = pair.second;
        oss << tool->getName() << ": " << tool->getDescription() << "\n";
    }
    
    return oss.str();
}

// 获取工具实例
std::shared_ptr<Tool> AIToolRegistry::getTool(const std::string& name) const {
    auto it = classTools_.find(name);
    if (it != classTools_.end()) {
        return it->second;
    }
    return nullptr;
}

// 验证工具参数
bool AIToolRegistry::validateToolArguments(const std::string& toolName, const json& args) const {
    auto tool = getTool(toolName);
    if (!tool) {
        // 如果找不到基于类的工具，认为参数验证通过（向后兼容）
        return true;
    }
    
    // 获取工具参数定义
    json schema = tool->getParameters();
    
    // 检查必需参数
    if (schema.contains("required") && schema["required"].is_array()) {
        for (const auto& requiredParam : schema["required"]) {
            std::string paramName = requiredParam.get<std::string>();
            // 检查参数是否存在且非空
            if (!args.contains(paramName) || args[paramName].is_null() ||
                (args[paramName].is_string() && args[paramName].get<std::string>().empty())) {
                return false;
            }
        }
    }
    
    return true;
}