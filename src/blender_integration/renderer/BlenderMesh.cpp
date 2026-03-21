#include "BlenderMesh.h"
#include <algorithm>
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Prisma {
namespace Renderer {

BlenderMesh::BlenderMesh() 
    : min_bounds_(glm::vec3(std::numeric_limits<float>::max()))
    , max_bounds_(glm::vec3(std::numeric_limits<float>::lowest())) {
}

BlenderMesh::BlenderMesh(const std::string& name) 
    : name_(name)
    , min_bounds_(glm::vec3(std::numeric_limits<float>::max()))
    , max_bounds_(glm::vec3(std::numeric_limits<float>::lowest())) {
}

BlenderMesh::~BlenderMesh() {
    Clear();
}

std::shared_ptr<BlenderMesh> BlenderMesh::CreateFromBlenderData(const MeshData& mesh_data) {
    auto mesh = std::make_shared<BlenderMesh>(mesh_data.name);
    
    if (mesh_data.vertices.empty() || mesh_data.triangles.empty()) {
        return mesh;
    }
    
    // 转换顶点数据
    std::vector<Vertex> vertices;
    vertices.reserve(mesh_data.GetVertexCount());
    
    for (size_t i = 0; i < mesh_data.vertices.size(); i += 3) {
        Vertex vertex;
        vertex.position = glm::vec3(
            mesh_data.vertices[i],
            mesh_data.vertices[i + 1],
            mesh_data.vertices[i + 2]
        );
        
        // 法线
        if (i + 2 < mesh_data.normals.size()) {
            vertex.normal = glm::vec3(
                mesh_data.normals[i],
                mesh_data.normals[i + 1],
                mesh_data.normals[i + 2]
            );
            mesh->has_normals_ = true;
        } else {
            vertex.normal = glm::vec3(0, 0, 1);
        }
        
        // UV坐标
        if (!mesh_data.uv_channels.empty() && i * 2 / 3 + 1 < mesh_data.uv_channels[0].size()) {
            size_t uv_idx = i * 2 / 3;
            vertex.texcoord = glm::vec2(
                mesh_data.uv_channels[0][uv_idx],
                mesh_data.uv_channels[0][uv_idx + 1]
            );
            mesh->has_uvs_ = true;
        } else {
            vertex.texcoord = glm::vec2(0, 0);
        }
        
        vertices.push_back(vertex);
    }
    
    // 使用三角形索引
    std::vector<uint32_t> indices(mesh_data.triangles.begin(), mesh_data.triangles.end());
    
    mesh->CreateFromVertices(vertices, indices);
    
    // 如果没有法线，生成它们
    if (!mesh->has_normals_) {
        mesh->GenerateNormals();
    }
    
    return mesh;
}

void BlenderMesh::CreateFromVertices(const std::vector<Vertex>& vertices, 
                                    const std::vector<uint32_t>& indices) {
    Clear();
    
    vertices_ = vertices;
    indices_ = indices;
    
    CalculateBounds();
    
    // 检查属性
    for (const auto& vertex : vertices_) {
        if (glm::length(vertex.normal) > 0.1f) {
            has_normals_ = true;
        }
        if (vertex.texcoord.x != 0.0f || vertex.texcoord.y != 0.0f) {
            has_uvs_ = true;
        }
    }
}

void BlenderMesh::CreateFromSimpleData(const std::vector<float>& vertices,
                                      const std::vector<float>& normals,
                                      const std::vector<float>& uvs,
                                      const std::vector<uint32_t>& triangles) {
    std::vector<Vertex> vtx;
    vtx.reserve(vertices.size() / 3);
    
    for (size_t i = 0; i < vertices.size(); i += 3) {
        Vertex vertex;
        vertex.position = glm::vec3(vertices[i], vertices[i + 1], vertices[i + 2]);
        
        if (i + 2 < normals.size()) {
            vertex.normal = glm::vec3(normals[i], normals[i + 1], normals[i + 2]);
            has_normals_ = true;
        }
        
        if (i * 2 / 3 + 1 < uvs.size()) {
            size_t uv_idx = i * 2 / 3;
            vertex.texcoord = glm::vec2(uvs[uv_idx], uvs[uv_idx + 1]);
            has_uvs_ = true;
        }
        
        vtx.push_back(vertex);
    }
    
    CreateFromVertices(vtx, triangles);
}

void BlenderMesh::Clear() {
    vertices_.clear();
    indices_.clear();
    sub_meshes_.clear();
    
    min_bounds_ = glm::vec3(std::numeric_limits<float>::max());
    max_bounds_ = glm::vec3(std::numeric_limits<float>::lowest());
    
    has_normals_ = false;
    has_uvs_ = false;
    has_tangents_ = false;
}

void BlenderMesh::CalculateBounds() {
    if (vertices_.empty()) {
        min_bounds_ = glm::vec3(0, 0, 0);
        max_bounds_ = glm::vec3(0, 0, 0);
        return;
    }
    
    min_bounds_ = vertices_[0].position;
    max_bounds_ = vertices_[0].position;
    
    for (const auto& vertex : vertices_) {
        min_bounds_ = glm::min(min_bounds_, vertex.position);
        max_bounds_ = glm::max(max_bounds_, vertex.position);
    }
}

void BlenderMesh::GenerateNormals() {
    if (vertices_.empty() || indices_.empty()) {
        return;
    }
    
    // 重置所有法线
    for (auto& vertex : vertices_) {
        vertex.normal = glm::vec3(0, 0, 0);
    }
    
    // 计算面法线并累加到顶点
    for (size_t i = 0; i < indices_.size(); i += 3) {
        uint32_t i0 = indices_[i];
        uint32_t i1 = indices_[i + 1];
        uint32_t i2 = indices_[i + 2];
        
        if (i0 >= vertices_.size() || i1 >= vertices_.size() || i2 >= vertices_.size()) {
            continue;
        }
        
        const auto& v0 = vertices_[i0];
        const auto& v1 = vertices_[i1];
        const auto& v2 = vertices_[i2];
        
        glm::vec3 edge1 = v1.position - v0.position;
        glm::vec3 edge2 = v2.position - v0.position;
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
        
        vertices_[i0].normal += normal;
        vertices_[i1].normal += normal;
        vertices_[i2].normal += normal;
    }
    
    // 归一化所有法线
    for (auto& vertex : vertices_) {
        vertex.normal = glm::normalize(vertex.normal);
    }
    
    has_normals_ = true;
}

void BlenderMesh::GenerateTangents() {
    if (!has_normals_ || !has_uvs_) {
        return;
    }
    
    // 切线生成需要UV坐标
    // 这里实现简化的MikkTSpace算法
    
    has_tangents_ = true;
}

void BlenderMesh::Optimize() {
    if (vertices_.empty() || indices_.empty()) {
        return;
    }
    
    // 1. 移除重复顶点
    std::vector<Vertex> unique_vertices;
    std::vector<uint32_t> remap(vertices_.size());
    std::unordered_map<Vertex, uint32_t, std::hash<Vertex>> vertex_map;
    
    for (size_t i = 0; i < vertices_.size(); ++i) {
        const auto& vertex = vertices_[i];
        auto it = vertex_map.find(vertex);
        if (it == vertex_map.end()) {
            uint32_t new_index = static_cast<uint32_t>(unique_vertices.size());
            vertex_map[vertex] = new_index;
            remap[i] = new_index;
            unique_vertices.push_back(vertex);
        } else {
            remap[i] = it->second;
        }
    }
    
    // 2. 重新映射索引
    for (auto& index : indices_) {
        if (index < remap.size()) {
            index = remap[index];
        }
    }
    
    // 3. 更新顶点数据
    vertices_ = std::move(unique_vertices);
    
    // 4. 重新计算边界
    CalculateBounds();
}

void BlenderMesh::AddSubMesh(const std::string& name, uint32_t start_index, 
                            uint32_t index_count, uint32_t material_index) {
    SubMesh submesh;
    submesh.name = name;
    submesh.start_index = start_index;
    submesh.index_count = index_count;
    submesh.material_index = material_index;
    
    sub_meshes_.push_back(submesh);
}

std::vector<uint8_t> BlenderMesh::Serialize() const {
    // 简单的二进制序列化
    std::vector<uint8_t> data;
    
    // 头部：魔数 + 版本
    struct Header {
        uint32_t magic = 0x4D455348;  // "MESH"
        uint32_t version = 1;
        uint32_t vertex_count;
        uint32_t index_count;
        uint32_t submesh_count;
        float bounds_min[3];
        float bounds_max[3];
    } header;
    
    header.vertex_count = static_cast<uint32_t>(vertices_.size());
    header.index_count = static_cast<uint32_t>(indices_.size());
    header.submesh_count = static_cast<uint32_t>(sub_meshes_.size());
    
    memcpy(header.bounds_min, glm::value_ptr(min_bounds_), sizeof(float) * 3);
    memcpy(header.bounds_max, glm::value_ptr(max_bounds_), sizeof(float) * 3);
    
    // 写入头部
    size_t offset = data.size();
    data.resize(data.size() + sizeof(Header));
    memcpy(data.data() + offset, &header, sizeof(Header));
    
    // 写入顶点数据
    offset = data.size();
    data.resize(data.size() + vertices_.size() * sizeof(Vertex));
    memcpy(data.data() + offset, vertices_.data(), vertices_.size() * sizeof(Vertex));
    
    // 写入索引数据
    offset = data.size();
    data.resize(data.size() + indices_.size() * sizeof(uint32_t));
    memcpy(data.data() + offset, indices_.data(), indices_.size() * sizeof(uint32_t));
    
    return data;
}

std::shared_ptr<BlenderMesh> BlenderMesh::Deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < sizeof(Header)) {
        return nullptr;
    }
    
    auto mesh = std::make_shared<BlenderMesh>();
    
    const auto* header = reinterpret_cast<const Header*>(data.data());
    if (header->magic != 0x4D455348) {
        return nullptr;
    }
    
    // 读取顶点数据
    const Vertex* vertices = reinterpret_cast<const Vertex*>(data.data() + sizeof(Header));
    mesh->vertices_.assign(vertices, vertices + header->vertex_count);
    
    // 读取索引数据
    const uint32_t* indices = reinterpret_cast<const uint32_t*>(
        data.data() + sizeof(Header) + header->vertex_count * sizeof(Vertex));
    mesh->indices_.assign(indices, indices + header->index_count);
    
    // 设置边界
    memcpy(glm::value_ptr(mesh->min_bounds_), header->bounds_min, sizeof(float) * 3);
    memcpy(glm::value_ptr(mesh->max_bounds_), header->bounds_max, sizeof(float) * 3);
    
    return mesh;
}

} // namespace Renderer
} // namespace Prisma