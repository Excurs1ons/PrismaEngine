#pragma once

#include "../ipc/SceneData.h"
#include <memory>
#include <vector>
#include <string>

namespace Prisma {
namespace Renderer {

// 顶点数据结构
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texcoord;
    glm::vec4 color;
    
    Vertex() : position(0,0,0), normal(0,0,1), texcoord(0,0), color(1,1,1,1) {}
    Vertex(const glm::vec3& pos, const glm::vec3& norm, const glm::vec2& uv)
        : position(pos), normal(norm), texcoord(uv), color(1,1,1,1) {}
};

// 网格类 - 用于渲染的网格数据
class BlenderMesh {
public:
    BlenderMesh();
    BlenderMesh(const std::string& name);
    ~BlenderMesh();
    
    // 从Blender数据创建网格
    static std::shared_ptr<BlenderMesh> CreateFromBlenderData(const MeshData& mesh_data);
    
    // 从顶点数据创建
    void CreateFromVertices(const std::vector<Vertex>& vertices, 
                           const std::vector<uint32_t>& indices);
    
    // 从简单数据创建
    void CreateFromSimpleData(const std::vector<float>& vertices,
                             const std::vector<float>& normals,
                             const std::vector<float>& uvs,
                             const std::vector<uint32_t>& indices);
    
    // 网格操作
    void Clear();
    bool IsValid() const { return !vertices_.empty() && !indices_.empty(); }
    
    // 属性获取
    const std::string& GetName() const { return name_; }
    size_t GetVertexCount() const { return vertices_.size(); }
    size_t GetIndexCount() const { return indices_.size(); }
    size_t GetTriangleCount() const { return indices_.size() / 3; }
    
    const std::vector<Vertex>& GetVertices() const { return vertices_; }
    const std::vector<uint32_t>& GetIndices() const { return indices_; }
    
    // 边界框
    const glm::vec3& GetMinBounds() const { return min_bounds_; }
    const glm::vec3& GetMaxBounds() const { return max_bounds_; }
    glm::vec3 GetCenter() const { return (min_bounds_ + max_bounds_) * 0.5f; }
    glm::vec3 GetSize() const { return max_bounds_ - min_bounds_; }
    
    // 优化
    void Optimize();
    void GenerateNormals();
    void GenerateTangents();
    
    // 子网格支持
    struct SubMesh {
        std::string name;
        uint32_t start_index;
        uint32_t index_count;
        uint32_t material_index;
    };
    
    void AddSubMesh(const std::string& name, uint32_t start_index, 
                   uint32_t index_count, uint32_t material_index = 0);
    const std::vector<SubMesh>& GetSubMeshes() const { return sub_meshes_; }
    
    // 序列化/反序列化
    std::vector<uint8_t> Serialize() const;
    static std::shared_ptr<BlenderMesh> Deserialize(const std::vector<uint8_t>& data);
    
private:
    void CalculateBounds();
    void CalculateNormals();
    
private:
    std::string name_;
    std::vector<Vertex> vertices_;
    std::vector<uint32_t> indices_;
    std::vector<SubMesh> sub_meshes_;
    
    // 边界框
    glm::vec3 min_bounds_;
    glm::vec3 max_bounds_;
    
    // 网格标志
    bool has_normals_{false};
    bool has_uvs_{false};
    bool has_tangents_{false};
};

} // namespace Renderer
} // namespace Prisma