#pragma once

#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <functional>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <queue>

#include "SceneData.h"

namespace Prisma {

// 消息类型枚举，与Python端保持同步
enum class BlenderMessageType : uint32_t {
    SCENE_UPDATE = 1,
    OBJECT_ADD = 2,
    OBJECT_REMOVE = 3,
    OBJECT_TRANSFORM = 4,
    MATERIAL_UPDATE = 5,
    CAMERA_UPDATE = 6,
    LIGHT_UPDATE = 7,
    SYNC_REQUEST = 8,
    SYNC_RESPONSE = 9,
    HEARTBEAT = 10
};

// 消息回调函数类型
using MessageCallback = std::function<void(const std::vector<uint8_t>&)>;
using SceneUpdateCallback = std::function<void(const SceneData&)>;

class BlenderIPCServer {
public:
    static std::shared_ptr<BlenderIPCServer> Get();
    
    BlenderIPCServer();
    ~BlenderIPCServer();
    
    // 服务器控制
    bool Start(const std::string& host = "127.0.0.1", uint16_t port = 12345);
    void Stop();
    bool IsRunning() const { return running_; }
    
    // 消息注册
    void RegisterMessageHandler(BlenderMessageType type, MessageCallback callback);
    void RegisterSceneUpdateHandler(SceneUpdateCallback callback);
    
    // 发送消息到Blender
    bool SendMessage(BlenderMessageType type, const std::vector<uint8_t>& data);
    bool SendSceneRequest();
    
    // 状态查询
    bool IsConnected() const { return client_connected_; }
    std::string GetConnectionInfo() const;
    
    // 队列处理
    void ProcessMessages();
    
private:
    // 服务器线程函数
    void ServerThread();
    void ClientHandler(int client_socket);
    
    // 消息处理
    void HandleMessage(BlenderMessageType type, const std::vector<uint8_t>& data);
    void HandleSceneUpdate(const std::vector<uint8_t>& data);
    
    // 序列化/反序列化
    SceneData DeserializeSceneData(const std::vector<uint8_t>& data);
    std::vector<uint8_t> SerializeSceneData(const SceneData& scene_data);
    
private:
    std::atomic<bool> running_{false};
    std::atomic<bool> client_connected_{false};
    
    int server_socket_{-1};
    int client_socket_{-1};
    
    std::thread server_thread_;
    std::thread client_thread_;
    
    std::mutex message_mutex_;
    std::queue<std::pair<BlenderMessageType, std::vector<uint8_t>>> message_queue_;
    
    std::mutex callback_mutex_;
    std::unordered_map<BlenderMessageType, MessageCallback> message_callbacks_;
    std::vector<SceneUpdateCallback> scene_update_callbacks_;
    
    std::string host_;
    uint16_t port_;
    
    // 序列化缓冲区
    static constexpr size_t BUFFER_SIZE = 1024 * 1024; // 1MB
    std::vector<uint8_t> serialization_buffer_;
};

} // namespace Prisma