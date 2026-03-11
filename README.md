# ChatServer

## 项目介绍
本项目是基于C++实现自定义的 HttpServer 框架，该项目包括 HTTP/HTTPS 支持、动态路由处理、会话管理等功能，基于该框架开发了一个 AI 对话助手。

### 核心模块介绍
1. **HttpServer框架**：自研的高性能HTTP服务器框架，支持HTTP/HTTPS协议，基于Reactor模式设计
2. **AI对话系统**：集成多种AI模型，支持文本对话和工具调用（如天气查询、时间获取等）
3. **会话管理**：完整的用户认证、会话维持和权限控制系统
4. **数据库访问**：封装MySQL数据库操作，支持连接池和ORM映射
5. **消息队列**：集成RabbitMQ实现异步任务处理
6. **实时通信**：通过SSE实现服务器推送功能
7. **配置管理**：统一的配置文件管理系统

## 项目环境
### 环境依赖
- 操作系统：Ubuntu 24.04
- 数据库：mysql 8.0
- boost_1_69_0
- muduo
- nlohmann/json
- libmysqlcppconn-dev
- openssl

### 配置环境
linux命令安装编译环境，需要时上网搜索相应命令。
#### 编译器相关
```sh
# 更新包管理器
sudo apt update

# 安装基础开发工具
sudo apt-get install -y build-essential cmake git libboost-all-dev
```
#### MySQL 安装
```sh
sudo apt-get install -y mysql-server
sudo systemctl start mysql
sudo systemctl enable mysql
# 创建数据库
# 导入数据库结构
```
#### RabbitMQ 安装
```sh
sudo apt-get install -y rabbitmq-server
sudo systemctl start rabbitmq-server
sudo systemctl enable rabbitmq-server
# 创建用户和虚拟主机
```
#### 第三方库
```sh
# boost
wget https://archives.boost.io/release/1.69.0/source/boost_1_69_0.tar.gz
tar -zxvf boost_1_69_0.tar.gz
cd boost_1_69_0
./bootstrap.sh
./b2
sudo ./b2 install -j $(nproc)
cd ..

# muduo
git clone https://github.com/chenshuo/muduo.git
cd muduo
./build.sh -j $(nproc)
sudo mkdir -p /usr/local/include/muduo
sudo cp -r muduo/base/* /usr/local/include/
sudo cp -r muduo/net/* /usr/local/include/
cd ..
sudo cp -r build/release-cpp11/lib/* /usr/local/lib/
sudo ldconfig

# nlohmann/json
sudo apt install nlohmann-json3-dev

# mysqlcppconn
sudo apt install libmysqlcppconn-dev

# librabbitmq
sudo apt install -y librabbitmq-dev

# OpenSSL
sudo apt install libssl-dev
```

## 项目编译
```sh
# 在项目根目录下创建build目录，并进入该目录
mkdir build
cd build
# 执行 cmake 命令
cmake ..
# 编译生成可执行文件
make -j $(nproc)
```

## 项目运行
### 导入数据库
```sh
# 导入数据库结构
mysql -u chatserver -p < db.sql
```

### 配置文件
ChatServer/resources/config.json 

### 服务启动
默认运行在 8080 端口，可以加上 -p 端口号来指定。
```
sudo ./chat_server
```

## 运行结果
服务启动后，浏览器访问自己对应的 ip:端口号

菜单页面
![菜单页面](images/image_2.png)

AI对话页面 
![AI对话页面](images/image_3.png)

## 总结

### 技术点
1. **高性能网络编程**：基于muduo网络库和Reactor模式，实现了高并发、低延迟的网络服务
2. **模块化设计**：采用模块化架构，各组件之间松耦合，便于维护和扩展
3. **AI集成能力**：深度集成多种AI服务，支持智能对话和工具调用
4. **实时通信支持**：通过SSE技术实现服务器主动推送，提升用户体验
5. **异步处理机制**：利用RabbitMQ实现耗时任务的异步处理，避免阻塞主线程
6. **安全机制**：完善的会话管理和用户认证体系，保障系统安全
7. **统一配置管理**：所有配置集中管理，便于部署和维护

### 设计模式应用
- **Reactor模式**：用于事件驱动的网络I/O处理
- **工厂模式**：AIFactory创建不同AI策略实例
- **策略模式**：AIStrategy支持多种AI模型切换
- **单例模式**：SessionManager、MQManager等全局管理器
- **观察者模式**：SSEChatHandler实现服务端推送

### 系统架构
```
客户端 ←→ HttpServer (Reactor + 多线程) ←→ 业务 Handler ←→ 数据库 / MQ / 第三方 API
```

组件交互流程:
1. 客户端发起 HTTP 请求 → HttpServer 接收并解析
2. Router 根据路径分发到对应 Handler
3. SessionManager 验证会话有效性
4. Handler 调用 AIHelper/AIFactory 获取 AI 响应
5. 结果通过 HttpResponse 或 SSE 流返回
6. 异步任务通过 RabbitMQ 解耦执行
