#pragma once

#include <chrono>
#include <random>
#include <string>
#include <memory>

class AISessionIdGenerator {
public:
    AISessionIdGenerator();
    
    std::string generate();

private:
    std::unique_ptr<std::mt19937_64> generator_;
};
