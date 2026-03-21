#pragma once

#include "BlenderProtocol.h"
#include <functional>
#include <unordered_map>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

namespace Prisma {

// JSON-RPC服务器，参考Unity Blender插件实现
class BlenderRPCServer {
public:
    using MessageHandler = std::function<BlenderProtocol::Message(const BlenderProtocol::Message&)>;
    using NotificationHandler = std::function<void(const BlenderProtocol::Message&)>;
    using ConnectionCallback = std::function<void(bool connected)>;
    
    static std::shared_ptr<BlenderRPCServer> Get();
    
    BlenderRPCServer();
    ~BlenderRPCServer();
    
    // 服务器控制
    bool Start(uint16_t port = 12345);
    void Stop();
    bool IsRunning() const { return running_; }
    bool IsConnected() const { return client_connected_; }
    
    // 方法注册
    void RegisterMethod(const std::string& method, MessageHandler handler);
    void RegisterNotification(BlenderProtocol::MessageType type, NotificationHandler handler);
    
    // 连接回调
    void SetConnectionCallback(ConnectionCallback callback);
    
    // 发送消息
    bool SendMessage(const BlenderProtocol::Message& message);
    bool SendNotification(BlenderProtocol::MessageType type, const nlohmann::json& params = {});
    
    // 调用远程方法
    std::future<BlenderProtocol::Message> CallMethod(const std::string& method, 
                                                     const nlohmann::json& params = {});
    
    // 状态查询
    std::string GetStatus() const;
    uint32_t GetMessageCount() const { return message_count_; }
    uint32_t GetErrorCount() const { return error_count_; }
    
private:
    // 服务器线程
    void ServerThread();
    void ClientHandler(int client_socket);
    
    // 消息处理
    void ProcessIncomingMessage(const BlenderProtocol::Message& message);
    BlenderProtocol::Message HandleRequest(const BlenderProtocol::Message& request);
    void HandleNotification(const BlenderProtocol::Message& notification);
    
    // 网络操作
    bool SendData(int socket, const std::vector<uint8_t>& data);
    std::vector<uint8_t> ReceiveData(int socket);
    std::vector<uint8_t> ReceiveExact(int socket, size_t size);
    
    // 消息队列
    void ProcessMessageQueue();
    
    // 心跳管理
    void StartHeartbeat();
    void StopHeartbeat();
    void HeartbeatThread();
    
private:
    std::atomic<bool> running_{false};
    std::atomic<bool> client_connected_{false};
    std::atomic<uint32_t> message_count_{0};
    std::atomic<uint32_t> error_count_{0};
    
    int server_socket_{-1};
    int client_socket_{-1};
    uint16_t port_{12345};
    
    std::thread server_thread_;
    std::thread client_thread_;
    std::thread heartbeat_thread_;
    
    // 消息处理
    std::mutex handler_mutex_;
    std::unordered_map<std::string, MessageHandler> method_handlers_;
    std::unordered_map<BlenderProtocol::MessageType, NotificationHandler> notification_handlers_;
    
    // 连接回调
    ConnectionCallback connection_callback_;
    
    // 消息队列
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::queue<BlenderProtocol::Message> incoming_queue_;
    std::queue<BlenderProtocol::Message> outgoing_queue_;
    
    // 请求-响应映射
    struct PendingRequest {
        std::promise<BlenderProtocol::Message> promise;
        std::chrono::steady_clock::time_point timeout;
    };
    
    std::mutex pending_mutex_;
    std::unordered_map<uint32_t, PendingRequest> pending_requests_;
    uint32_t next_message_id_{1};
    
    // 心跳
    std::atomic<bool> heartbeat_running_{false};
    std::chrono::milliseconds heartbeat_interval_{5000};  // 5秒
    
    // 统计
    std::chrono::steady_clock::time_point start_time_;
};

// 默认方法处理器
class DefaultRPCHandlers {
public:
    // 场景操作
    static BlenderProtocol::Message HandleSceneOpen(const BlenderProtocol::Message& request);
    static BlenderProtocol::Message HandleSceneClose(const BlenderProtocol::Message& request);
    static BlenderProtocol::Message HandleSceneSave(const BlenderProtocol::Message& request);
    
    // 对象操作
    static BlenderProtocol::Message HandleObjectCreate(const BlenderProtocol::Message& request);
    static BlenderProtocol::Message HandleObjectDelete(const BlenderProtocol::Message& request);
    static BlenderProtocol::Message HandleObjectUpdate(const BlenderProtocol::Message& request);
    static BlenderProtocol::Message HandleObjectTransform(const BlenderProtocol::Message& request);
    
    // 同步
    static BlenderProtocol::Message HandleSyncRequest(const BlenderProtocol::Message& request);
    static BlenderProtocol::Message HandleSyncResponse(const BlenderProtocol::Message& request);
    
    // 系统
    static BlenderProtocol::Message HandlePing(const BlenderProtocol::Message& request);
    static BlenderProtocol::Message HandleHeartbeat(const BlenderProtocol::Message& request);
    
    // 通知处理器
    static void HandleSceneUpdateNotification(const BlenderProtocol::Message& notification);
    static void HandleSelectionUpdateNotification(const BlenderProtocol::Message& notification);
    static void HandleObjectTransformNotification(const BlenderProtocol::Message& notification);
};

} // namespace Prisma