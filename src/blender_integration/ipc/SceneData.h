#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <glm/glm.hpp>

namespace Prisma {

// 基础数学类型
struct Vector3 {
    float x, y, z;
    
    Vector3() : x(0), y(0), z(0) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
    
    glm::vec3 ToGlm() const { return glm::vec3(x, y, z); }
    static Vector3 FromGlm(const glm::vec3& v) { return Vector3(v.x, v.y, v.z); }
};

struct Quaternion {
    float x, y, z, w;
    
    Quaternion() : x(0), y(0), z(0), w(1) {}
    Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    
    glm::quat ToGlm() const { return glm::quat(w, x, y, z); }
    static Quaternion FromGlm(const glm::quat& q) { return Quaternion(q.x, q.y, q.z, q.w); }
};

struct Transform {
    Vector3 position;
    Quaternion rotation;
    Vector3 scale;
    
    Transform() : scale(1, 1, 1) {}
    
    glm::mat4 ToMatrix() const {
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), position.ToGlm());
        glm::mat4 rotation_mat = glm::mat4_cast(rotation.ToGlm());
        glm::mat4 scale_mat = glm::scale(glm::mat4(1.0f), scale.ToGlm());
        return translation * rotation_mat * scale_mat;
    }
};

// 网格数据
struct MeshData {
    std::string name;
    std::vector<float> vertices;      // x, y, z
    std::vector<float> normals;       // nx, ny, nz
    std::vector<std::vector<float>> uv_channels; // 多个UV通道
    std::vector<uint32_t> triangles;  // 三角形索引
    
    bool HasNormals() const { return !normals.empty(); }
    bool HasUVs() const { return !uv_channels.empty(); }
    size_t GetVertexCount() const { return vertices.size() / 3; }
    size_t GetTriangleCount() const { return triangles.size() / 3; }
};

// 材质数据
struct MaterialData {
    std::string name;
    
    // PBR材质属性
    glm::vec3 albedo_color = glm::vec3(0.8f, 0.8f, 0.8f);
    float roughness = 0.5f;
    float metallic = 0.0f;
    
    // 纹理路径
    std::unordered_map<std::string, std::string> textures; // 纹理类型 -> 文件路径
    
    // 透明度
    float alpha = 1.0f;
    bool transparent = false;
    
    // 着色器信息
    std::string shader_name = "PBR";
    
    bool HasTexture(const std::string& type) const {
        return textures.find(type) != textures.end();
    }
};

// 对象类型
enum class ObjectType {
    MESH,
    CAMERA,
    LIGHT,
    EMPTY
};

// 相机类型
enum class CameraType {
    PERSPECTIVE,
    ORTHOGRAPHIC
};

// 光源类型
enum class LightType {
    POINT,
    DIRECTIONAL,
    SPOT,
    AREA
};

// 基础对象数据
struct ObjectData {
    std::string id;
    std::string name;
    ObjectType type = ObjectType::MESH;
    Transform transform;
    
    // 类型特定数据
    std::shared_ptr<MeshData> mesh_data;
    std::shared_ptr<MaterialData> material_data;
    
    // 相机特定数据
    struct {
        CameraType camera_type = CameraType::PERSPECTIVE;
        float fov = 60.0f;           // 视野角度
        float near_plane = 0.1f;
        float far_plane = 100.0f;
        float ortho_size = 5.0f;     // 正交相机大小
    } camera_props;
    
    // 光源特定数据
    struct {
        LightType light_type = LightType::POINT;
        glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
        float intensity = 1.0f;
        float range = 10.0f;         // 点光源/聚光灯范围
        float spot_angle = 45.0f;    // 聚光灯角度
        float inner_spot_angle = 30.0f;
    } light_props;
    
    // 元数据
    std::unordered_map<std::string, std::string> metadata;
    
    bool IsMesh() const { return type == ObjectType::MESH; }
    bool IsCamera() const { return type == ObjectType::CAMERA; }
    bool IsLight() const { return type == ObjectType::LIGHT; }
    bool IsEmpty() const { return type == ObjectType::EMPTY; }
};

// 场景数据
struct SceneData {
    std::string name = "Untitled";
    
    // 场景对象
    std::vector<ObjectData> objects;
    std::vector<ObjectData> cameras;
    std::vector<ObjectData> lights;
    
    // 活动相机
    std::string active_camera_id;
    
    // 环境设置
    glm::vec3 ambient_light = glm::vec3(0.1f, 0.1f, 0.1f);
    std::string skybox_texture;
    
    // 场景元数据
    struct {
        std::string author;
        std::string created_date;
        std::string modified_date;
        std::string description;
        std::string blender_version;
        std::string prisma_version = "1.0.0";
    } metadata;
    
    // 辅助方法
    ObjectData* FindObjectById(const std::string& id) {
        for (auto& obj : objects) {
            if (obj.id == id) return &obj;
        }
        return nullptr;
    }
    
    ObjectData* FindCameraById(const std::string& id) {
        for (auto& cam : cameras) {
            if (cam.id == id) return &cam;
        }
        return nullptr;
    }
    
    ObjectData* FindLightById(const std::string& id) {
        for (auto& light : lights) {
            if (light.id == id) return &light;
        }
        return nullptr;
    }
    
    void Clear() {
        objects.clear();
        cameras.clear();
        lights.clear();
        active_camera_id.clear();
    }
};

} // namespace Prisma