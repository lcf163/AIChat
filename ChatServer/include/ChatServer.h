#pragma once

#include <memory>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <vector>

#include "http/HttpServer.h"
#include "utils/MysqlUtil.h"
#include "utils/FileUtil.h"
#include "utils/JsonUtil.h"

#include "utils/base64.h"
#include "utils/MQManager.h"
#include "utils/ThreadPool.h"
#include "core/AIHelper.h"

class ChatLoginHandler;
class ChatRegisterHandler;
class ChatLogoutHandler;
class ChatHandler;
class ChatEntryHandler;
class ChatSendHandler;
class ChatSpeechHandler;
class ChatSessionsHandler;
class ChatHistoryHandler;
class ChatCreateAndSendHandler;
class SSEChatHandler;

class AIMenuHandler;

class ChatServer {
public:
	ChatServer(int port,
		const std::string& name,
		muduo::net::TcpServer::Option option = muduo::net::TcpServer::kNoReusePort);

	void start();

	void setThreadNum(int numThreads);

	http::session::SessionManager* getSessionManager() const
	{
		return httpServer_.getSessionManager();
	}

	void setSessionManager(std::unique_ptr<http::session::SessionManager> manager)
	{
		httpServer_.setSessionManager(std::move(manager));
	}
	
	// 添加获取server指针的方法，供SSEChatHandler使用
	ChatServer* getServer() { return this; }
	
	// SSE 连接管理接口
	// 在线用户管理方法 (无锁实现)
	void addUser(int userId);
	void removeUser(int userId);
	bool isUserOnline(int userId) const;
	
	// 聊天信息管理方法 (无锁实现)
	void addOrUpdateChatSession(int userId, const std::string& sessionId, std::shared_ptr<AIHelper> helper);
	std::shared_ptr<AIHelper> getChatSession(int userId, const std::string& sessionId);
	void removeChatSession(int userId, const std::string& sessionId);
	
	// 会话ID管理方法 (无锁实现)
	void addSessionId(int userId, const std::string& sessionId);
	void removeSessionId(int userId, const std::string& sessionId);
	std::vector<std::string> getSessionIds(int userId) const;
	
	// 聊天结果管理方法 (无锁实现)
	void setChatResult(const std::string& sessionId, const std::string& result);
	std::string getChatResult(const std::string& sessionId);
	void removeChatResult(const std::string& sessionId);
	
	// LRU Cache相关方法 (无锁实现)
	void updateLRUCache(int userId, const std::string& sessionId);
	void evictLRUCacheIfNeeded();
	
	// SSE 连接管理方法 (无锁实现)
	void addSSEConnection(const std::string& sessionId, const muduo::net::TcpConnectionPtr& conn);
	void removeSSEConnection(const std::string& sessionId);
	void sendSSEData(const std::string& sessionId, const std::string& data, const std::string& eventType = "result");
	
	// 获取业务线程池引用，供Handlers使用
	std::shared_ptr<ThreadPool> getBusinessThreadPool() { return businessThreadPool_; }
	
private:
	friend class ChatLoginHandler;
	friend class ChatRegisterHandler;
	friend class ChatLogoutHandler;
	friend class ChatHandler;
	friend class ChatEntryHandler;
	friend class ChatSendHandler;
	friend class ChatSpeechHandler;
	friend class ChatHistoryHandler;
	friend class ChatSessionsHandler;
	friend class ChatCreateAndSendHandler;
	friend class SSEChatHandler;
	friend class AIMenuHandler;

private:
	void initialize();
	void initializeSession();
	void initializeRouter();
	void initializeMiddleware();
	
	void loadSessionsFromDatabase();

	void packageResp(const std::string& version, http::HttpResponse::HttpStatusCode statusCode,
		const std::string& statusMsg, bool close, const std::string& contentType,
		int contentLen, const std::string& body, http::HttpResponse* resp);

	http::HttpServer httpServer_;

	http::MysqlUtil mysqlUtil_;

	// 在线用户管理 (无锁实现)
	std::unordered_map<int, bool> onlineUsers_;
	
	// 聊天信息管理 (无锁实现)
	std::unordered_map<int, std::unordered_map<std::string, std::shared_ptr<AIHelper>>> chatInformation;

	// 会话ID管理 (无锁实现)
	std::unordered_map<int, std::vector<std::string>> sessionsIdsMap;
	
	// 添加业务线程池，用于处理AI请求
	std::shared_ptr<ThreadPool> businessThreadPool_;
	
	// 存储聊天结果的容器 (无锁实现)
	std::unordered_map<std::string, std::string> chatResults;
	
	// SSE 连接映射表 (无锁实现)
	std::unordered_map<std::string, muduo::net::TcpConnectionPtr> sseConnections_;
	
	// LRU Cache相关成员 (无锁实现)
	std::list<std::pair<int, std::string>> lruCacheList_;
	std::unordered_map<std::string, std::list<std::pair<int, std::string>>::iterator> lruCacheMap_;
};