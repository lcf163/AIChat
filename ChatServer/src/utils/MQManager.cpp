#include "utils/MQManager.h"
#include "core/AIConfig.h"
#include <muduo/base/Logging.h>

// ------------------- MQManager -------------------
MQManager::MQManager(size_t poolSize)
    : poolSize_(poolSize), counter_(0) {
    // 从配置中获取RabbitMQ信息
    const auto& mqConfig = AIConfig::getInstance().getRabbitMQConfig();
    
    try {
        for (size_t i = 0; i < poolSize_; ++i) {
            auto conn = std::make_shared<MQConn>();
            conn->channel = AmqpClient::Channel::Create(
                mqConfig.host, 
                mqConfig.port, 
                mqConfig.username, 
                mqConfig.password, 
                mqConfig.vhost);

            pool_.push_back(conn);
        }
        LOG_INFO << "RabbitMQ connection pool initialized with " << poolSize_ << " connections";
    } catch (const std::exception& e) {
        LOG_ERROR << "Failed to initialize RabbitMQ connection pool: " << e.what();
        LOG_WARN << "RabbitMQ is not available, continuing without message queue functionality";
        // 清空连接池，后续操作会检查连接池是否为空
        pool_.clear();
        poolSize_ = 0;
    }
}

void MQManager::publish(const std::string& queue, const std::string& msg) {
    if (pool_.empty()) {
        LOG_WARN << "RabbitMQ pool is empty, skipping publish: " << msg.substr(0, 100) << "...";
        return;
    }
    
    size_t index = counter_.fetch_add(1) % poolSize_;
    auto& conn = pool_[index];

    try {
        std::lock_guard<std::mutex> lock(conn->mtx);
        auto message = AmqpClient::BasicMessage::Create(msg);
        conn->channel->BasicPublish("", queue, message);
    } catch (const std::exception& e) {
        LOG_ERROR << "Failed to publish message to RabbitMQ: " << e.what();
    }
}

// ------------------- RabbitMQThreadPool -------------------
void RabbitMQThreadPool::start() {
    for (int i = 0; i < thread_num_; ++i) {
        workers_.emplace_back(&RabbitMQThreadPool::worker, this, i);
    }
}

void RabbitMQThreadPool::shutdown() {
    stop_ = true;
    for (auto& t : workers_) {
        if (t.joinable()) t.join();
    }
}

void RabbitMQThreadPool::worker(int id) {
    try {
        // 从配置中获取RabbitMQ信息
        const auto& mqConfig = AIConfig::getInstance().getRabbitMQConfig();
        
        // Each thread has its own independent channel
        auto channel = AmqpClient::Channel::Create(
            rabbitmq_host_, 
            mqConfig.port, 
            mqConfig.username, 
            mqConfig.password, 
            mqConfig.vhost);
            
        channel->DeclareQueue(queue_name_, false, true, false, false);
        std::string consumer_tag = channel->BasicConsume(queue_name_, "", true, false, false);

        channel->BasicQos(consumer_tag, 1); 

        while (!stop_) {
            AmqpClient::Envelope::ptr_t env;
            bool ok = channel->BasicConsumeMessage(consumer_tag, env, 500); // 500ms 
            if (ok && env) {
                std::string msg = env->Message()->Body();
                handler_(msg);          
                channel->BasicAck(env); 
            }
        }

        channel->BasicCancel(consumer_tag);
    }
    catch (const std::exception& e) {
        LOG_ERROR << "Thread " << id << " exception: " << e.what();
    }
}