#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <stdexcept>
#include <iostream>
#include <memory>

#include "utils/JsonUtil.h"
#include "Tool.h"

class AIToolRegistry {
public:
    using ToolFunc = std::function<json(const json&)>;

    AIToolRegistry();

    // 注册函数式工具
    void registerTool(const std::string& name, ToolFunc func);
    
    // 注册基于Tool类的工具
    void registerTool(std::shared_ptr<Tool> tool);
    
    // 调用工具
    json invoke(const std::string& name, const json& args) const;
    
    // 检查是否存在某个工具
    bool hasTool(const std::string& name) const;
    
    // 获取工具列表信息（用于构建提示词）
    std::string getToolListDescription() const;
    
    // 获取工具实例
    std::shared_ptr<Tool> getTool(const std::string& name) const;
    
    // 验证工具参数
    bool validateToolArguments(const std::string& toolName, const json& args) const;

private:
    std::unordered_map<std::string, ToolFunc> functionTools_;
    std::unordered_map<std::string, std::shared_ptr<Tool>> classTools_;
};