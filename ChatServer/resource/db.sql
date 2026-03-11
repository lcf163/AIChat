-- =============================================
-- 数据库建表语句
-- =============================================

-- 创建数据库
CREATE DATABASE IF NOT EXISTS ChatHttpServer DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE ChatHttpServer;

-- =============================================
-- 用户表 (users)
-- 存储用户基本信息，用于登录注册功能
-- =============================================
DROP TABLE IF EXISTS users;
CREATE TABLE users (
    id INT AUTO_INCREMENT PRIMARY KEY COMMENT '用户ID，主键自增',
    username VARCHAR(50) NOT NULL UNIQUE COMMENT '用户名，唯一索引',
    password VARCHAR(255) NOT NULL COMMENT '密码，存储加密后的值',
    salt VARCHAR(255) NOT NULL COMMENT '密码盐值',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    INDEX idx_username (username) COMMENT '用户名索引，用于快速查询',
    INDEX idx_password (password) COMMENT '密码索引，用于快速查询',
    INDEX idx_salt (salt) COMMENT '密码盐值索引，用于快速查询'
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='用户表';

-- =============================================
-- 聊天会话表 (chat_session)
-- 存储用户的聊天会话信息，用于会话管理
-- =============================================
DROP TABLE IF EXISTS chat_session;
CREATE TABLE chat_session (
    id BIGINT AUTO_INCREMENT PRIMARY KEY COMMENT '会话ID，主键自增',
    user_id INT NOT NULL COMMENT '用户ID，关联users表',
    username VARCHAR(50) NOT NULL COMMENT '用户名',
    session_id VARCHAR(100) NOT NULL UNIQUE COMMENT '会话唯一标识符',
    title VARCHAR(200) DEFAULT '新对话' COMMENT '会话标题',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '会话创建时间',
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '会话最后更新时间',
    INDEX idx_user_id (user_id) COMMENT '用户ID索引',
    INDEX idx_username (username) COMMENT '用户名索引',
    INDEX idx_session_id (session_id) COMMENT '会话ID索引'
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='聊天会话表';

-- =============================================
-- 聊天消息表 (chat_message)
-- 存储用户的聊天记录，包括用户消息和AI回复
-- =============================================
DROP TABLE IF EXISTS chat_message;
CREATE TABLE chat_message (
    id BIGINT AUTO_INCREMENT PRIMARY KEY COMMENT '消息ID，主键自增',
		user_id INT NOT NULL,
    username VARCHAR(50) NOT NULL COMMENT '发送消息的用户名',
    session_id VARCHAR(100) NOT NULL COMMENT '会话ID，用于分组同一会话的消息',
    is_user TINYINT(1) NOT NULL DEFAULT 1 COMMENT '是否为用户消息：1表示用户消息，0表示AI回复',
    content TEXT NOT NULL COMMENT '消息内容',
    ts BIGINT NOT NULL COMMENT '时间戳（毫秒），用于消息排序',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '记录创建时间',
		INDEX idx_user (user_id),
		INDEX idx_session (session_id),
    INDEX idx_session_id (session_id) COMMENT '会话ID索引，用于查询会话历史',
    INDEX idx_ts (ts) COMMENT '时间戳索引，用于时间排序',
    INDEX idx_username (username) COMMENT '用户名索引，用于查询用户消息'
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='聊天消息表';