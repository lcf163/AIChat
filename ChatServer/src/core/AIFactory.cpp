#include "core/AIFactory.h"
#include "core/BaiduSpeechService.h"

StrategyFactory& StrategyFactory::instance() {
    static StrategyFactory factory;
    return factory;
}

void StrategyFactory::registerStrategy(const std::string& name, Creator creator) {
    creators_[name] = std::move(creator);
}

std::shared_ptr<AIStrategy> StrategyFactory::create(const std::string& name) {
    auto it = creators_.find(name);
    if (it == creators_.end()) {
        throw std::runtime_error("Unknown strategy: " + name);
    }
    return it->second();
}

std::unique_ptr<SpeechService> SpeechServiceFactory::createSpeechService(SpeechServiceProvider provider,
                                                                       const std::string& clientId,
                                                                       const std::string& clientSecret) {
    switch (provider) {
        case SpeechServiceProvider::BAIDU:
            return std::make_unique<BaiduSpeechService>(clientId, clientSecret);
        case SpeechServiceProvider::UNKNOWN:
        default:
            return nullptr;
    }
}