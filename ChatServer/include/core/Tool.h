#pragma once

#include <string>
#include <memory>
#include "utils/JsonUtil.h"

/**
 * @brief 工具基类，所有AI工具都应该继承此类
 */
class Tool {
public:
    virtual ~Tool() = default;
    
    /**
     * @brief 获取工具名称
     * @return 工具名称
     */
    virtual std::string getName() const = 0;
    
    /**
     * @brief 获取工具描述
     * @return 工具描述
     */
    virtual std::string getDescription() const = 0;
    
    /**
     * @brief 获取工具参数定义
     * @return 参数定义的JSON格式
     */
    virtual json getParameters() const = 0;
    
    /**
     * @brief 执行工具
     * @param args 工具参数
     * @return 执行结果
     */
    virtual json execute(const json& args) const = 0;
};