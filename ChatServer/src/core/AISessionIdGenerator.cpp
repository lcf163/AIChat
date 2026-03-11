#include "core/AISessionIdGenerator.h"
#include <sstream>
#include <iomanip>
AISessionIdGenerator::AISessionIdGenerator() : generator_(std::make_unique<std::mt19937_64>()) {
    auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    generator_->seed(seed);
}

std::string AISessionIdGenerator::generate(){
    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    std::uniform_int_distribution<uint64_t> distribution;
    uint64_t randVal = distribution(*generator_);
    
    // Combine timestamp and random value to create a more unique identifier
    uint64_t rawId = static_cast<uint64_t>(now) ^ randVal;
    
    // Convert to hex string for better distribution and readability
    std::stringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << rawId;
    return ss.str();
}