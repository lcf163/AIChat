#pragma once

#include "Tool.h"
#include <ctime>

/**
 * @brief 时间查询工具
 */
class TimeTool : public Tool {
public:
    std::string getName() const override;
    std::string getDescription() const override;
    json getParameters() const override;
    json execute(const json& args) const override;
};