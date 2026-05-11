#include "BlenderIPCServer.h"
#include "SceneData.h"

#include <cstring>
#include <iostream>
#include <sstream>
#include <iomanip>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #define SOCKET_ERROR_CODE WSAGetLastError()
    #define CLOSE_SOCKET closesocket
    #define SOCKET_TYPE SOCKET
    using socklen_t = int;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #define SOCKET_ERROR_CODE errno
    #define CLOSE_SOCKET close
    #define SOCKET_TYPE int
    #define INVALID_SOCKET -1
#endif

#include "Logger.h"

namespace Prisma {

std::shared_ptr<BlenderIPCServer> BlenderIPCServer::Get() {
    static std::shared_ptr<BlenderIPCServer> instance = std::make_shared<BlenderIPCServer>();
    return instance;
}

BlenderIPCServer::BlenderIPCServer() 
    : host_("127.0.0.1")
    , port_(12345)
    , serialization_buffer_(BUFFER_SIZE) {
    
#ifdef _WIN32
    // 初始化Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        LOG_ERROR("BlenderIPCServer", "WSAStartup failed");
    }
#endif
}

BlenderIPCServer::~BlenderIPCServer() {
    Stop();
    
#ifdef _WIN32
    WSACleanup();
#endif
}

bool BlenderIPCServer::Start(const std::string& host, uint16_t port) {
    if (running_) {
        LOG_WARNING("BlenderIPCServer", "Server already running");
        return true;
    }
    
    host_ = host;
    port_ = port;
    
    // 创建服务器socket
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ == INVALID_SOCKET) {
        LOG_ERROR("BlenderIPCServer", "Failed to create socket: {}", SOCKET_ERROR_CODE);
        return false;
    }
    
    // 设置socket选项
    int opt = 1;
#ifdef _WIN32
    if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, 
                   reinterpret_cast<const char*>(&opt), sizeof(opt)) < 0) {
        LOG_WARNING("BlenderIPCServer", "Failed to set SO_REUSEADDR");
    }
#else
    if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        LOG_WARNING("BlenderIPCServer", "Failed to set SO_REUSEADDR");
    }
#endif
    
    // 绑定地址
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);
    
    if (host_ == "127.0.0.1" || host_ == "localhost") {
        server_addr.sin_addr.s_addr = INADDR_ANY;
    } else {
        server_addr.sin_addr.s_addr = inet_addr(host_.c_str());
    }
    
    if (bind(server_socket_, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        LOG_ERROR("BlenderIPCServer", "Failed to bind socket: {}", SOCKET_ERROR_CODE);
        CLOSE_SOCKET(server_socket_);
        return false;
    }
    
    // 开始监听
    if (listen(server_socket_, 1) < 0) {
        LOG_ERROR("BlenderIPCServer", "Failed to listen: {}", SOCKET_ERROR_CODE);
        CLOSE_SOCKET(server_socket_);
        return false;
    }
    
    // 启动服务器线程
    running_ = true;
    server_thread_ = std::thread(&BlenderIPCServer::ServerThread, this);
    
    LOG_INFO("BlenderIPCServer", "IPC server started on {}:{}", host_, port_);
    return true;
}

void BlenderIPCServer::Stop() {
    if (!running_) return;
    
    running_ = false;
    
    // 关闭客户端连接
    if (client_socket_ != INVALID_SOCKET) {
        CLOSE_SOCKET(client_socket_);
        client_socket_ = INVALID_SOCKET;
    }
    
    // 关闭服务器socket
    if (server_socket_ != INVALID_SOCKET) {
        CLOSE_SOCKET(server_socket_);
        server_socket_ = INVALID_SOCKET;
    }
    
    // 等待线程结束
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
    
    if (client_thread_.joinable()) {
        client_thread_.join();
    }
    
    client_connected_ = false;
    LOG_INFO("BlenderIPCServer", "IPC server stopped");
}

void BlenderIPCServer::RegisterMessageHandler(BlenderMessageType type, MessageCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    message_callbacks_[type] = std::move(callback);
}

void BlenderIPCServer::RegisterSceneUpdateHandler(SceneUpdateCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    scene_update_callbacks_.push_back(std::move(callback));
}

bool BlenderIPCServer::SendMessage(BlenderMessageType type, const std::vector<uint8_t>& data) {
    if (!client_connected_ || client_socket_ == INVALID_SOCKET) {
        LOG_WARNING("BlenderIPCServer", "Cannot send message: client not connected");
        return false;
    }
    
    // 构造消息头
    struct MessageHeader {
        uint32_t type;
        uint32_t size;
    } header;
    
    header.type = static_cast<uint32_t>(type);
    header.size = static_cast<uint32_t>(data.size());
    
    // 发送消息头
    int sent = send(client_socket_, reinterpret_cast<const char*>(&header), sizeof(header), 0);
    if (sent != sizeof(header)) {
        LOG_ERROR("BlenderIPCServer", "Failed to send message header: {}", SOCKET_ERROR_CODE);
        return false;
    }
    
    // 发送消息体
    if (!data.empty()) {
        sent = send(client_socket_, reinterpret_cast<const char*>(data.data()), data.size(), 0);
        if (sent != static_cast<int>(data.size())) {
            LOG_ERROR("BlenderIPCServer", "Failed to send message body: {}", SOCKET_ERROR_CODE);
            return false;
        }
    }
    
    return true;
}

bool BlenderIPCServer::SendSceneRequest() {
    // 发送同步请求
    std::vector<uint8_t> empty_data;
    return SendMessage(BlenderMessageType::SYNC_REQUEST, empty_data);
}

std::string BlenderIPCServer::GetConnectionInfo() const {
    std::stringstream ss;
    ss << "Server: " << host_ << ":" << port_ 
       << ", Connected: " << (client_connected_ ? "Yes" : "No")
       << ", Running: " << (running_ ? "Yes" : "No");
    return ss.str();
}

void BlenderIPCServer::ProcessMessages() {
    std::lock_guard<std::mutex> lock(message_mutex_);
    
    while (!message_queue_.empty()) {
        auto& [type, data] = message_queue_.front();
        HandleMessage(type, data);
        message_queue_.pop();
    }
}

void BlenderIPCServer::ServerThread() {
    LOG_INFO("BlenderIPCServer", "Server thread started");
    
    while (running_) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        
        LOG_INFO("BlenderIPCServer", "Waiting for Blender connection...");
        
        // 接受客户端连接
        client_socket_ = accept(server_socket_, 
                               reinterpret_cast<sockaddr*>(&client_addr), 
                               &client_len);
        
        if (client_socket_ == INVALID_SOCKET) {
            if (running_) {
                LOG_ERROR("BlenderIPCServer", "Accept failed: {}", SOCKET_ERROR_CODE);
            }
            continue;
        }
        
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        
        LOG_INFO("BlenderIPCServer", "Blender connected from {}:{}", 
                 client_ip, ntohs(client_addr.sin_port));
        
        client_connected_ = true;
        
        // 启动客户端处理线程
        client_thread_ = std::thread(&BlenderIPCServer::ClientHandler, this, client_socket_);
        
        // 等待客户端线程结束
        if (client_thread_.joinable()) {
            client_thread_.join();
        }
        
        client_connected_ = false;
        LOG_INFO("BlenderIPCServer", "Blender disconnected");
    }
}

void BlenderIPCServer::ClientHandler(int client_socket) {
    LOG_INFO("BlenderIPCServer", "Client handler started");
    
    while (client_connected_ && running_) {
        // 接收消息头
        struct MessageHeader {
            uint32_t type;
            uint32_t size;
        } header;
        
        int received = recv(client_socket, reinterpret_cast<char*>(&header), sizeof(header), 0);
        
        if (received <= 0) {
            if (received == 0) {
                LOG_INFO("BlenderIPCServer", "Client disconnected gracefully");
            } else {
                LOG_ERROR("BlenderIPCServer", "Failed to receive message header: {}", SOCKET_ERROR_CODE);
            }
            break;
        }
        
        if (received != sizeof(header)) {
            LOG_ERROR("BlenderIPCServer", "Incomplete message header received");
            continue;
        }
        
        // 接收消息体
        std::vector<uint8_t> data(header.size);
        if (header.size > 0) {
            size_t total_received = 0;
            while (total_received < header.size) {
                received = recv(client_socket, 
                               reinterpret_cast<char*>(data.data() + total_received),
                               header.size - total_received, 0);
                
                if (received <= 0) {
                    LOG_ERROR("BlenderIPCServer", "Failed to receive message body");
                    break;
                }
                total_received += received;
            }
        }
        
        if (data.size() == header.size) {
            // 将消息加入队列
            std::lock_guard<std::mutex> lock(message_mutex_);
            message_queue_.emplace(static_cast<BlenderMessageType>(header.type), std::move(data));
        }
    }
    
    CLOSE_SOCKET(client_socket);
    client_socket_ = INVALID_SOCKET;
}

void BlenderIPCServer::HandleMessage(BlenderMessageType type, const std::vector<uint8_t>& data) {
    // 调用注册的回调函数
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        auto it = message_callbacks_.find(type);
        if (it != message_callbacks_.end()) {
            it->second(data);
        }
    }
    
    // 特殊处理场景更新
    if (type == BlenderMessageType::SCENE_UPDATE) {
        HandleSceneUpdate(data);
    }
    
    LOG_DEBUG("BlenderIPCServer", "Handled message type: {}, size: {} bytes", 
              static_cast<uint32_t>(type), data.size());
}

void BlenderIPCServer::HandleSceneUpdate(const std::vector<uint8_t>& data) {
    try {
        SceneData scene_data = DeserializeSceneData(data);
        
        std::lock_guard<std::mutex> lock(callback_mutex_);
        for (const auto& callback : scene_update_callbacks_) {
            callback(scene_data);
        }
        
        LOG_INFO("BlenderIPCServer", "Scene updated: {} objects, {} cameras, {} lights",
                 scene_data.objects.size(), scene_data.cameras.size(), scene_data.lights.size());
    } catch (const std::exception& e) {
        LOG_ERROR("BlenderIPCServer", "Failed to deserialize scene data: {}", e.what());
    }
}

SceneData BlenderIPCServer::DeserializeSceneData(const std::vector<uint8_t>& data) {
    // 简化实现，实际需要完整的序列化/反序列化
    // 这里使用JSON作为示例
    SceneData scene_data;
    
    // TODO: 实现完整的序列化/反序列化
    // 目前返回空场景
    return scene_data;
}

std::vector<uint8_t> BlenderIPCServer::SerializeSceneData(const SceneData& scene_data) {
    // TODO: 实现完整的序列化
    return std::vector<uint8_t>();
}

} // namespace Prisma