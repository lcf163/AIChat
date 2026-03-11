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

std::vector<std::string> StrategyFactory::getRegisteredModels() const {
    std::vector<std::string> models;
    for (const auto& [name, _] : creators_) {
        models.push_back(name);
    }
    return models;
}

bool StrategyFactory::hasModel(const std::string& name) const {
    return creators_.find(name) != creators_.end();
}

std::string StrategyFactory::getModelInfo(const std::string& name) const {
    auto it = creators_.find(name);
    if (it == creators_.end()) {
        return "Model not found: " + name;
    }
    try {
        auto strategy = it->second();
        return strategy->getModelInfo();
    } catch (const std::exception& e) {
        return "Error getting model info: " + std::string(e.what());
    }
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