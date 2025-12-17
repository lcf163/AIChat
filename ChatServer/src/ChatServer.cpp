#include <muduo/base/Logging.h>
#include <thread> // 用于获取CPU核心数

#include "http/HttpRequest.h"
#include "http/HttpResponse.h"
#include "http/HttpServer.h"

#include "handlers/ChatEntryHandler.h"
#include "handlers/ChatLoginHandler.h"
#include "handlers/ChatRegisterHandler.h"
#include "handlers/ChatLogoutHandler.h"
#include "handlers/ChatHandler.h"
#include "handlers/ChatSendHandler.h"
#include "handlers/ChatSessionsHandler.h"
#include "handlers/ChatHistoryHandler.h"
#include "handlers/ChatCreateAndSendHandler.h"
#include "handlers/SSEChatHandler.h"
#include "handlers/ChatSpeechHandler.h"
#include "handlers/AIMenuHandler.h"
#include "core/AIConfig.h"
#include "ChatServer.h"

using namespace http;

ChatServer::ChatServer(int port,
    const std::string& name,
    muduo::net::TcpServer::Option option)
    : httpServer_(port, name, option)
{
    initialize();
}

void ChatServer::initialize() {
    LOG_INFO << "ChatServer initialize start !";
    
    // 从配置中获取数据库信息
    const auto& dbConfig = AIConfig::getInstance().getDatabaseConfig();
    
    // 初始化 MysqlUtil
	http::MysqlUtil::init(dbConfig.host, dbConfig.user, dbConfig.password, dbConfig.database, dbConfig.poolSize);
    
    // 初始化 Session（包含会话列表懒加载）
    initializeSession();

    // 初始化中间件
    initializeMiddleware();

    // 初始化路由接口，请求通过 Handler 做相应业务处理
    initializeRouter();

    // 初始化业务线程池，用于处理AI请求
    // 线程数设置为CPU核心数+1，但最少4个，最多16个
    size_t threadCount = std::thread::hardware_concurrency() + 1;
    threadCount = std::max(threadCount, static_cast<size_t>(4));
    threadCount = std::min(threadCount, static_cast<size_t>(16));
    
    LOG_INFO << "Initializing business thread pool with " << threadCount << " threads";
    businessThreadPool_ = std::make_shared<ThreadPool>(threadCount);

    LOG_INFO << "ChatServer initialize success !";
}

void ChatServer::loadSessionsFromDatabase() {
    LOG_INFO << "Loading sessions from database...";
    
    try {
        std::string sql = "SELECT user_id, session_id FROM chat_session ORDER BY user_id, created_at";
        std::unique_ptr<sql::ResultSet> result(mysqlUtil_.executeQuery(sql));
        
        if (!result) {
            LOG_INFO << "No sessions found in database (first startup?)";
            return;
        }

        // 使用 runInLoop 在 IO 线程中执行操作
        std::promise<void> prom;
        auto fut = prom.get_future();
        
        httpServer_.getLoop()->runInLoop([this, &result, &prom]() {
            int sessionCount = 0;
            while (result->next()) {
                int userId = result->getInt("user_id");
                std::string sessionId = result->getString("session_id");

                // 只加载会话ID列表，不加载消息内容（懒加载策略）
                auto& sessionList = sessionsIdsMap[userId];
                if (std::find(sessionList.begin(), sessionList.end(), sessionId) == sessionList.end()) {
                    sessionList.push_back(sessionId);
                }
                
                sessionCount++;
            }
            
            LOG_INFO << "Loaded " << sessionCount << " sessions (messages will load on demand)";
            prom.set_value();
        });
        
        fut.wait();
    }
    catch (const std::exception& e) {
        LOG_ERROR << "Error loading sessions from database: " << e.what();
    }
    catch (...) {
        LOG_ERROR << "Unknown error occurred while loading sessions from database";
    }
}


void ChatServer::setThreadNum(int numThreads) {
    httpServer_.setThreadNum(numThreads);
}

void ChatServer::start() {
    httpServer_.start();
}

void ChatServer::initializeRouter() {
    httpServer_.Get("/", std::make_shared<ChatEntryHandler>(this));
    httpServer_.Get("/entry", std::make_shared<ChatEntryHandler>(this));
    
    httpServer_.Post("/login", std::make_shared<ChatLoginHandler>(this));
    httpServer_.Post("/register", std::make_shared<ChatRegisterHandler>(this));
    httpServer_.Post("/user/logout", std::make_shared<ChatLogoutHandler>(this));

    httpServer_.Get("/chat", std::make_shared<ChatHandler>(this));
    httpServer_.Post("/chat/send", std::make_shared<ChatSendHandler>(this));
    httpServer_.Post("/chat/tts", std::make_shared<ChatSpeechHandler>(this));
    httpServer_.Get("/chat/sessions", std::make_shared<ChatSessionsHandler>(this));
    httpServer_.Post("/chat/history", std::make_shared<ChatHistoryHandler>(this));
    httpServer_.Post("/chat/send-new-session", std::make_shared<ChatCreateAndSendHandler>(this));
    httpServer_.Get("/chat/stream", std::make_shared<SSEChatHandler>(this));
 
    httpServer_.Get("/menu", std::make_shared<AIMenuHandler>(this));
}

void ChatServer::initializeSession() {
    auto sessionStorage = std::make_unique<http::session::MemorySessionStorage>();
    auto sessionManager = std::make_unique<http::session::SessionManager>(std::move(sessionStorage));

    setSessionManager(std::move(sessionManager));
    loadSessionsFromDatabase();
}

void ChatServer::updateLRUCache(int userId, const std::string& sessionId) {
    httpServer_.getLoop()->runInLoop([this, userId, sessionId]() {
        // 如果会话已在LRU缓存中，先移除旧条目
        auto it = lruCacheMap_.find(sessionId);
        if (it != lruCacheMap_.end()) {
            lruCacheList_.erase(it->second);
            lruCacheMap_.erase(it);
        }
        
        // 将新会话添加到LRU链表头部（表示最近使用）
        lruCacheList_.emplace_front(userId, sessionId);
        lruCacheMap_[sessionId] = lruCacheList_.begin();
        
        // 检查并执行LRU淘汰策略
        evictLRUCacheIfNeeded();
    });
}

void ChatServer::evictLRUCacheIfNeeded() {
    // 从配置中获取最大活跃会话数
    const auto& limitsConfig = AIConfig::getInstance().getLimitsConfig();
    const size_t MAX_ACTIVE_SESSIONS = limitsConfig.maxActiveSessions;
    
    // 当活跃会话数超过最大限制时，开始淘汰
    while (lruCacheList_.size() > MAX_ACTIVE_SESSIONS) {
        // 获取并移除LRU链表尾部元素（最久未使用的会话）
        auto last = lruCacheList_.back();
        int userId = last.first;
        std::string sessionId = last.second;
        
        lruCacheMap_.erase(sessionId);
        lruCacheList_.pop_back();
        
        // 从内存中的聊天信息里移除该会话
        auto userIt = chatInformation.find(userId);
        if (userIt != chatInformation.end()) {
            userIt->second.erase(sessionId);
            // 如果该用户的所有会话都被移除，则清理用户条目
            if (userIt->second.empty()) {
                chatInformation.erase(userIt);
            }
        }
        
        LOG_INFO << "LRU Cache Eviction: Removed session '" << sessionId 
                 << "' for user " << userId;
    }
}

void ChatServer::initializeMiddleware() {
    // 添加 CORS 中间件
    auto corsMiddleware = std::make_shared<http::middleware::CorsMiddleware>();
    httpServer_.addMiddleware(corsMiddleware);
}

void ChatServer::packageResp(const std::string& version, http::HttpResponse::HttpStatusCode statusCode,
	const std::string& statusMsg, bool close, const std::string& contentType,
	int contentLen, const std::string& body, http::HttpResponse* resp) {

	resp->setStatusLine(version, statusCode, statusMsg);
	resp->setCloseConnection(close);
	resp->setContentType(contentType);
	resp->setContentLength(contentLen);
	resp->setBody(body);
}

void ChatServer::addSSEConnection(const std::string& sessionId, const muduo::net::TcpConnectionPtr& conn) {
    httpServer_.getLoop()->runInLoop([this, sessionId, conn]() {
        sseConnections_[sessionId] = conn;
    });
}

void ChatServer::removeSSEConnection(const std::string& sessionId) {
    httpServer_.getLoop()->runInLoop([this, sessionId]() {
        sseConnections_.erase(sessionId);
    });
}

void ChatServer::sendSSEData(const std::string& sessionId, const std::string& data, const std::string& eventType) {
    httpServer_.getLoop()->runInLoop([this, sessionId, data, eventType]() {
        auto it = sseConnections_.find(sessionId);
        if (it != sseConnections_.end() && it->second && it->second->connected()) {
            std::ostringstream oss;
            oss << "event: " << eventType << "\n";
            
            if (eventType == "result") {
                json j;
                j["result"] = data; 
                oss << "data: " << j.dump() << "\n\n";
            } else {
                oss << "data: " << data << "\n\n";
            }
            
            it->second->send(oss.str());
        }
    });
}

void ChatServer::addUser(int userId) {
    httpServer_.getLoop()->runInLoop([this, userId]() {
        onlineUsers_[userId] = true;
    });
}

void ChatServer::removeUser(int userId) {
    httpServer_.getLoop()->runInLoop([this, userId]() {
        onlineUsers_.erase(userId);
    });
}

bool ChatServer::isUserOnline(int userId) const {
    // 使用 promise/future 机制在 IO 线程中获取结果
    std::promise<bool> prom;
    auto fut = prom.get_future();
    
    httpServer_.getLoop()->runInLoop([this, userId, &prom]() {
        auto it = onlineUsers_.find(userId);
        prom.set_value(it != onlineUsers_.end() && it->second);
    });
    
    fut.wait();
    return fut.get();
}

void ChatServer::addSessionId(int userId, const std::string& sessionId) {
    httpServer_.getLoop()->runInLoop([this, userId, sessionId]() {
        auto& sessionList = sessionsIdsMap[userId];
        if (std::find(sessionList.begin(), sessionList.end(), sessionId) == sessionList.end()) {
            sessionList.push_back(sessionId);
        }
    });
}

void ChatServer::removeSessionId(int userId, const std::string& sessionId) {
    httpServer_.getLoop()->runInLoop([this, userId, sessionId]() {
        auto userIt = sessionsIdsMap.find(userId);
        if (userIt != sessionsIdsMap.end()) {
            auto& sessionList = userIt->second;
            sessionList.erase(
                std::remove(sessionList.begin(), sessionList.end(), sessionId),
                sessionList.end()
            );
            if (sessionList.empty()) {
                sessionsIdsMap.erase(userIt);
            }
        }
    });
}

std::vector<std::string> ChatServer::getSessionIds(int userId) const {
    std::promise<std::vector<std::string>> prom;
    auto fut = prom.get_future();
    
    httpServer_.getLoop()->runInLoop([this, userId, &prom]() {
        auto it = sessionsIdsMap.find(userId);
        if (it != sessionsIdsMap.end()) {
            prom.set_value(it->second);
        } else {
            prom.set_value(std::vector<std::string>());
        }
    });
    
    fut.wait();
    return fut.get();
}

void ChatServer::addOrUpdateChatSession(int userId, const std::string& sessionId, std::shared_ptr<AIHelper> helper) {
    httpServer_.getLoop()->runInLoop([this, userId, sessionId, helper]() {
        chatInformation[userId][sessionId] = helper;
    });
}

std::shared_ptr<AIHelper> ChatServer::getChatSession(int userId, const std::string& sessionId) {
    std::promise<std::shared_ptr<AIHelper>> prom;
    auto fut = prom.get_future();
    
    httpServer_.getLoop()->runInLoop([this, userId, sessionId, &prom]() {
        auto userIt = chatInformation.find(userId);
        if (userIt != chatInformation.end()) {
            auto sessionIt = userIt->second.find(sessionId);
            if (sessionIt != userIt->second.end()) {
                prom.set_value(sessionIt->second);
                return;
            }
        }
        prom.set_value(nullptr);
    });
    
    fut.wait();
    return fut.get();
}

void ChatServer::removeChatSession(int userId, const std::string& sessionId) {
    httpServer_.getLoop()->runInLoop([this, userId, sessionId]() {
        auto userIt = chatInformation.find(userId);
        if (userIt != chatInformation.end()) {
            userIt->second.erase(sessionId);
            if (userIt->second.empty()) {
                chatInformation.erase(userIt);
            }
        }
    });
}

void ChatServer::setChatResult(const std::string& sessionId, const std::string& result) {
    httpServer_.getLoop()->runInLoop([this, sessionId, result]() {
        chatResults[sessionId] = result;
        
        // AI处理完成，发送结果到对应的SSE连接
        auto it = sseConnections_.find(sessionId);
        if (it != sseConnections_.end() && it->second && it->second->connected()) {
            // 发送结果事件
            std::ostringstream resultData;
            resultData << "event: result\n";
            
            json j;
            j["result"] = result; 
            resultData << "data: " << j.dump() << "\n\n";
            
            it->second->send(resultData.str());
            
            // 发送结束事件
            std::ostringstream endData;
            endData << "event: end\n";
            endData << "data: {\"message\": \"AI processing completed\"}\n\n";
            
            it->second->send(endData.str());
        }
        
        // 移除聊天结果，释放内存
        chatResults.erase(sessionId);
    });
}

std::string ChatServer::getChatResult(const std::string& sessionId) {
    std::promise<std::string> prom;
    auto fut = prom.get_future();
    
    httpServer_.getLoop()->runInLoop([this, sessionId, &prom]() {
        auto it = chatResults.find(sessionId);
        if (it != chatResults.end()) {
            prom.set_value(it->second);
        } else {
            prom.set_value("");
        }
    });
    
    fut.wait();
    return fut.get();
}

void ChatServer::removeChatResult(const std::string& sessionId) {
    httpServer_.getLoop()->runInLoop([this, sessionId]() {
        chatResults.erase(sessionId);
    });
}
