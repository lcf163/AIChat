#pragma once

#include <string>
#include <vector>
#include <utility>
#include <iostream>
#include <sstream>
#include <memory>
#include <functional>
#include <unordered_map>

#include "AIStrategy.h"
#include "AIConfig.h"
#include "SpeechService.h"

class StrategyFactory {
public:
    using Creator = std::function<std::shared_ptr<AIStrategy>()>;

    static StrategyFactory& instance();

    void registerStrategy(const std::string& name, Creator creator);

    std::shared_ptr<AIStrategy> create(const std::string& name);

private:
    StrategyFactory() = default;
    std::unordered_map<std::string, Creator> creators_;
};

template<typename T>
struct StrategyRegister {
    explicit StrategyRegister(const std::string& name) {
        StrategyFactory::instance().registerStrategy(name, []() -> std::shared_ptr<AIStrategy> {
            return std::make_shared<T>();
        });
    }
};

/**
 * 语音服务工厂类
 * 根据配置创建对应的语音服务实例
 */
class SpeechServiceFactory {
public:
    static std::unique_ptr<SpeechService> createSpeechService(SpeechServiceProvider provider,
                                                              const std::string& clientId,
                                                              const std::string& clientSecret);
};