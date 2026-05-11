#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace Prisma {

// 基于JSON-RPC的通信协议，参考Unity Blender插件实现
namespace BlenderProtocol {

// 消息类型
enum class MessageType : uint32_t {
    // 场景操作
    SCENE_OPEN = 1,
    SCENE_CLOSE = 2,
    SCENE_SAVE = 3,
    
    // 对象操作
    OBJECT_CREATE = 10,
    OBJECT_DELETE = 11,
    OBJECT_UPDATE = 12,
    OBJECT_TRANSFORM = 13,
    OBJECT_SELECT = 14,
    
    // 组件操作
    MESH_UPDATE = 20,
    MATERIAL_UPDATE = 21,
    CAMERA_UPDATE = 22,
    LIGHT_UPDATE = 23,
    
    // 同步
    SYNC_REQUEST = 30,
    SYNC_RESPONSE = 31,
    SYNC_COMPLETE = 32,
    
    // 编辑操作
    UNDO = 40,
    REDO = 41,
    
    // 系统
    PING = 100,
    PONG = 101,
    ERROR = 102,
    HEARTBEAT = 103
};

// 错误代码
enum class ErrorCode : int {
    SUCCESS = 0,
    INVALID_MESSAGE = 1,
    UNSUPPORTED_OPERATION = 2,
    OBJECT_NOT_FOUND = 3,
    PERMISSION_DENIED = 4,
    INTERNAL_ERROR = 5,
    CONNECTION_LOST = 6,
    TIMEOUT = 7
};

// JSON-RPC消息结构
struct Message {
    MessageType type;
    uint32_t id;  // 消息ID，用于请求-响应匹配
    std::string method;  // JSON-RPC方法名
    nlohmann::json params;  // 方法参数
    nlohmann::json result;  // 方法结果（仅响应）
    ErrorCode error_code;  // 错误代码
    std::string error_message;  // 错误信息
    
    // 序列化/反序列化
    std::vector<uint8_t> Serialize() const;
    static Message Deserialize(const std::vector<uint8_t>& data);
    
    // 快速创建消息
    static Message CreateRequest(MessageType type, const std::string& method, 
                                 const nlohmann::json& params = {});
    static Message CreateResponse(uint32_t request_id, const nlohmann::json& result = {});
    static Message CreateError(uint32_t request_id, ErrorCode code, 
                               const std::string& message = "");
    
    // 检查消息类型
    bool IsRequest() const { return !method.empty() && result.empty(); }
    bool IsResponse() const { return method.empty() && !result.empty(); }
    bool IsError() const { return error_code != ErrorCode::SUCCESS; }
};

// 场景数据结构
struct SceneInfo {
    std::string name;
    std::string path;
    uint32_t object_count;
    uint32_t material_count;
    uint32_t light_count;
    uint32_t camera_count;
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(SceneInfo, name, path, object_count, 
                                  material_count, light_count, camera_count)
};

// 变换数据
struct TransformData {
    float position[3];
    float rotation[4];  // quaternion: x, y, z, w
    float scale[3];
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(TransformData, position, rotation, scale)
};

// 网格数据
struct MeshData {
    std::string name;
    std::vector<float> vertices;  // x, y, z
    std::vector<float> normals;   // nx, ny, nz
    std::vector<float> uvs;       // u, v
    std::vector<uint32_t> triangles;
    std::vector<uint32_t> materials;
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(MeshData, name, vertices, normals, uvs, 
                                  triangles, materials)
};

// 材质数据
struct MaterialData {
    std::string name;
    std::string shader;
    std::map<std::string, float> float_params;
    std::map<std::string, std::array<float, 3>> color_params;
    std::map<std::string, std::string> texture_params;
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(MaterialData, name, shader, float_params, 
                                  color_params, texture_params)
};

// 相机数据
struct CameraData {
    std::string name;
    TransformData transform;
    float fov;
    float near_clip;
    float far_clip;
    bool orthographic;
    float ortho_size;
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(CameraData, name, transform, fov, near_clip, 
                                  far_clip, orthographic, ortho_size)
};

// 光源数据
struct LightData {
    std::string name;
    TransformData transform;
    std::string type;  // "POINT", "DIRECTIONAL", "SPOT", "AREA"
    std::array<float, 3> color;
    float intensity;
    float range;
    float spot_angle;
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(LightData, name, transform, type, color, 
                                  intensity, range, spot_angle)
};

// 对象数据
struct ObjectData {
    std::string id;
    std::string name;
    std::string type;  // "MESH", "CAMERA", "LIGHT", "EMPTY"
    TransformData transform;
    std::optional<MeshData> mesh;
    std::optional<MaterialData> material;
    std::optional<CameraData> camera;
    std::optional<LightData> light;
    std::map<std::string, std::string> properties;
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ObjectData, id, name, type, transform, mesh, 
                                  material, camera, light, properties)
};

// 场景更新消息
struct SceneUpdate {
    std::vector<ObjectData> objects;
    std::vector<std::string> removed_objects;
    std::vector<ObjectData> cameras;
    std::vector<ObjectData> lights;
    std::string active_camera_id;
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(SceneUpdate, objects, removed_objects, 
                                  cameras, lights, active_camera_id)
};

// 选择消息
struct SelectionUpdate {
    std::vector<std::string> selected_objects;
    std::string active_object;
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(SelectionUpdate, selected_objects, active_object)
};

// 操作结果
struct OperationResult {
    bool success;
    std::string message;
    std::optional<std::string> object_id;
    std::optional<nlohmann::json> data;
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(OperationResult, success, message, object_id, data)
};

// 协议版本
constexpr uint32_t PROTOCOL_VERSION = 1;
constexpr uint32_t MIN_SUPPORTED_VERSION = 1;

// 消息头
struct MessageHeader {
    uint32_t magic = 0x50524953;  // "PRIS"
    uint32_t version = PROTOCOL_VERSION;
    uint32_t type;
    uint32_t id;
    uint32_t size;
    uint32_t checksum;
    
    static bool Validate(const MessageHeader& header);
    static uint32_t CalculateChecksum(const std::vector<uint8_t>& data);
};

// 协议助手函数
class ProtocolHelper {
public:
    // 创建标准消息
    static Message CreateSceneOpenMessage(const std::string& scene_path);
    static Message CreateSceneUpdateMessage(const SceneUpdate& update);
    static Message CreateObjectTransformMessage(const std::string& object_id, 
                                               const TransformData& transform);
    static Message CreateSelectionUpdateMessage(const SelectionUpdate& selection);
    static Message CreatePingMessage();
    static Message CreateHeartbeatMessage();
    
    // 解析消息
    static SceneUpdate ParseSceneUpdate(const Message& message);
    static TransformData ParseTransformUpdate(const Message& message);
    static SelectionUpdate ParseSelectionUpdate(const Message& message);
    
    // 验证消息
    static bool ValidateMessage(const Message& message);
    static std::string GetErrorMessage(ErrorCode code);
};

} // namespace BlenderProtocol

} // namespace Prisma