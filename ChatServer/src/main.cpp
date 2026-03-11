#include <string>
#include <iostream>
#include <thread>
#include <chrono>
#include <muduo/net/TcpServer.h>
#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>

#include "utils/JsonUtil.h"
#include "core/AIConfig.h"
#include "ChatServer.h"

// 工作线程绑定的函数
void executeMysql(const std::string sql_with_params) {
    // 从配置中获取数据库信息
    const auto& dbConfig = AIConfig::getInstance().getDatabaseConfig();
    
    http::MysqlUtil mysqlUtil_;
    
    try {
        // 尝试解析为JSON格式（新格式）
        json messageJson = json::parse(sql_with_params);
        
        if (messageJson.contains("sql") && messageJson.contains("params")) {
            std::string sql = messageJson["sql"];
            json params = messageJson["params"];
            
            // 根据参数数量执行相应的executeUpdate
            switch (params.size()) {
                case 1:
                    mysqlUtil_.executeUpdate(sql, params[0].get<std::string>());
                    break;
                case 2:
                    mysqlUtil_.executeUpdate(sql, params[0].get<std::string>(), params[1].get<std::string>());
                    break;
                case 3:
                    mysqlUtil_.executeUpdate(sql, params[0].get<std::string>(), params[1].get<std::string>(), params[2].get<std::string>());
                    break;
                case 4:
                    mysqlUtil_.executeUpdate(sql, params[0].get<std::string>(), params[1].get<std::string>(), params[2].get<std::string>(), params[3].get<std::string>());
                    break;
                case 5:
                    mysqlUtil_.executeUpdate(sql, params[0].get<std::string>(), params[1].get<std::string>(), params[2].get<std::string>(), params[3].get<std::string>(), params[4].get<std::string>());
                    break;
                case 6:
                    mysqlUtil_.executeUpdate(sql, params[0].get<std::string>(), params[1].get<std::string>(), params[2].get<std::string>(), params[3].get<std::string>(), params[4].get<std::string>(), params[5].get<std::string>());
                    break;
                default:
                    LOG_ERROR << "Unsupported number of parameters: " << params.size();
                    break;
            }
        } else {
            LOG_ERROR << "Invalid JSON format: missing sql or params";
        }
    } catch (const std::exception& e) {
        LOG_ERROR << "Error parsing JSON message: " << e.what() << ", message: " << sql_with_params;
    }
}

int main(int argc, char* argv[]) {
	std::string serverName = "ChatServer";
	int port = 8080;
    int opt;
    const char* str = "p:";
    while ((opt = getopt(argc, argv, str)) != -1)
    {
        switch (opt)
        {
        case 'p':
        {
            port = atoi(optarg);
            break;
        }
        default:
            break;
        }
    }

    // 加载配置文件
    AIConfig& config = AIConfig::getInstance();
    if (!config.loadFromFile("../ChatServer/resource/config.json")) {
        LOG_ERROR << "Failed to load config file!";
        return -1;
    }
    
    // 根据配置设置日志级别
    const auto& logConfig = config.getLogConfig();
    switch (logConfig.level) {
        case LogLevel::TRACE:
            muduo::Logger::setLogLevel(muduo::Logger::TRACE);
            break;
        case LogLevel::DEBUG:
            muduo::Logger::setLogLevel(muduo::Logger::DEBUG);
            break;
        case LogLevel::INFO:
            muduo::Logger::setLogLevel(muduo::Logger::INFO);
            break;
        case LogLevel::WARN:
            muduo::Logger::setLogLevel(muduo::Logger::WARN);
            break;
        case LogLevel::ERROR:
            muduo::Logger::setLogLevel(muduo::Logger::ERROR);
            break;
        case LogLevel::FATAL:
            muduo::Logger::setLogLevel(muduo::Logger::FATAL);
            break;
        default:
            muduo::Logger::setLogLevel(muduo::Logger::WARN);
            break;
    }
    
    // 在设置日志级别后再打印进程ID
    LOG_INFO << "pid = " << getpid();

    // 初始化 ChatServer
    ChatServer server(port, serverName);
    // server.setThreadNum(4);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // 从配置中获取RabbitMQ信息
    const auto& mqConfig = config.getRabbitMQConfig();
    
    // 初始化线程池，绑定参数
    RabbitMQThreadPool pool(mqConfig.host, mqConfig.queueName, mqConfig.threadNum, executeMysql);
    // 启动线程池，作为消费者处理 MQ 的消息
    pool.start();

    server.start();
    
    return 0;
}