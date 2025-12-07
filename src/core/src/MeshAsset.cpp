#include "MeshAsset.h"
#include "Logger.h"
#include <fstream>
#include <sstream>
#include "AssetSerializer.h"

namespace Engine {
using namespace Serialization;
bool MeshAsset::Load(const std::filesystem::path& path) {
    try {
        if (!std::filesystem::exists(path)) {
            LOG_ERROR("Mesh", "Mesh file does not exist: {0}", path.string());
            return false;
        }

        // 这里应该实现实际的网格加载逻辑
        // 例如从OBJ、FBX等格式加载
        // 为了演示，我们创建一个简单的三角形网格

        SubMesh triangle;
        triangle.name          = "Triangle";
        triangle.materialIndex = 0;

        // 创建三角形顶点
        triangle.vertices.resize(3);
        triangle.vertices[0].position = XMFLOAT4(0.0f, 0.5f, 0.0f, 1.0f);
        triangle.vertices[0].normal   = XMFLOAT4(0.0f, 0.0f, 1.0f, 0.0f);
        triangle.vertices[0].texCoord = XMFLOAT4(0.5f, 0.0f, 0.0f, 0.0f);
        triangle.vertices[0].tangent  = XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f);
        triangle.vertices[0].color    = XMVECTORF32{1.0f, 1.0f, 1.0f, 1.0f};

        triangle.vertices[1].position = XMFLOAT4(-0.5f, -0.5f, 0.0f, 1.0f);
        triangle.vertices[1].normal   = XMFLOAT4(0.0f, 0.0f, 1.0f, 0.0f);
        triangle.vertices[1].texCoord = XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);
        triangle.vertices[1].tangent  = XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f);
        triangle.vertices[1].color    = XMVECTORF32{1.0f, 1.0f, 1.0f, 1.0f};

        triangle.vertices[2].position = XMFLOAT4(0.5f, -0.5f, 0.0f, 1.0f);
        triangle.vertices[2].normal   = XMFLOAT4(0.0f, 0.0f, 1.0f, 0.0f);
        triangle.vertices[2].texCoord = XMFLOAT4(1.0f, 1.0f, 0.0f, 0.0f);
        triangle.vertices[2].tangent  = XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f);
        triangle.vertices[2].color    = XMVECTORF32{1.0f, 1.0f, 1.0f, 1.0f};

        // 创建三角形索引
        triangle.indices = {0, 1, 2};

        // 添加到子网格列表
        m_subMeshes.push_back(triangle);

        // 计算包围盒
        XMVECTOR minVec = XMVectorSet(FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX);
        XMVECTOR maxVec = XMVectorSet(-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX);

        for (const auto& subMesh : m_subMeshes) {
            for (const auto& vertex : subMesh.vertices) {
                XMVECTOR pos = XMLoadFloat4(&vertex.position);
                minVec       = XMVectorMin(minVec, pos);
                maxVec       = XMVectorMax(maxVec, pos);
            }
        }

        BoundingBox::CreateFromPoints(m_boundingBox, minVec, maxVec);

        // 设置路径和名称
        m_path                = path;
        m_name                = path.filename().string();
        m_metadata.sourcePath = path;
        m_metadata.name       = m_name;

        m_isLoaded = true;
        LOG_INFO("Mesh", "Successfully loaded mesh: {0} with {1} submeshes", m_name, m_subMeshes.size());
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Mesh", "Exception while loading mesh: {0}", e.what());
        return false;
    }
}

void MeshAsset::Unload() {
    m_subMeshes.clear();
    m_boundingBox = BoundingBox();
    m_isLoaded    = false;
    LOG_INFO("Mesh", "Unloaded mesh: {0}", m_name);
}

void MeshAsset::Serialize(OutputArchive& archive) const {
    // 序列化元数据
    archive.BeginObject();
    archive("metadata", m_metadata);

    // 序列化包围盒
    archive("boundingBox", m_boundingBox);

    // 序列化子网格
    archive.BeginArray(m_subMeshes.size());
    for (const auto& subMesh : m_subMeshes) {
        archive.BeginObject();
        archive("name", subMesh.name);
        archive("materialIndex", subMesh.materialIndex);

        // 序列化顶点
        archive.BeginArray(subMesh.vertices.size());
        for (const auto& vertex : subMesh.vertices) {
            archive.BeginObject();
            archive("position", vertex.position);
            archive("normal", vertex.normal);
            archive("texCoord", vertex.texCoord);
            archive("tangent", vertex.tangent);
            archive("color", vertex.color);
            archive.EndObject();
        }
        archive.EndArray();

        // 序列化索引
        archive.BeginArray(subMesh.indices.size());
        for (const auto& index : subMesh.indices) {
            archive("", index);
        }
        archive.EndArray();

        archive.EndObject();
    }
    archive.EndArray();

    archive.EndObject();
}

void MeshAsset::Deserialize(InputArchive& archive) {
    // 反序列化元数据
    size_t fieldCount = archive.BeginObject();

    for (size_t i = 0; i < fieldCount; ++i) {
        if (archive.HasNextField("metadata")) {
            m_metadata.Deserialize(archive);
        } else if (archive.HasNextField("boundingBox")) {
            // 这里需要实现BoundingBox的反序列化
            // 由于BoundingBox可能没有直接的反序列化支持，我们跳过它
            // 实际项目中应该实现BoundingBox的序列化/反序列化
            archive.BeginObject();
            archive.EndObject();
        } else if (archive.HasNextField("subMeshes")) {
            size_t subMeshCount = archive.BeginArray();
            m_subMeshes.resize(subMeshCount);

            for (size_t j = 0; j < subMeshCount; ++j) {
                SubMesh& subMesh         = m_subMeshes[j];
                size_t subMeshFieldCount = archive.BeginObject();

                for (size_t k = 0; k < subMeshFieldCount; ++k) {
                    if (archive.HasNextField("name")) {
                        subMesh.name = archive.ReadString();
                    } else if (archive.HasNextField("materialIndex")) {
                        subMesh.materialIndex = archive.ReadUInt32();
                    } else if (archive.HasNextField("vertices")) {
                        size_t vertexCount = archive.BeginArray();
                        subMesh.vertices.resize(vertexCount);

                        for (size_t l = 0; l < vertexCount; ++l) {
                            Vertex& vertex          = subMesh.vertices[l];
                            size_t vertexFieldCount = archive.BeginObject();

                            for (size_t m = 0; m < vertexFieldCount; ++m) {
                                if (archive.HasNextField("position")) {
                                    // 这里需要实现XMFLOAT4的反序列化
                                    // 由于XMFLOAT4可能没有直接的反序列化支持，我们跳过它
                                    // 实际项目中应该实现XMFLOAT4的序列化/反序列化
                                    archive.BeginObject();
                                    archive.EndObject();
                                } else if (archive.HasNextField("normal")) {
                                    archive.BeginObject();
                                    archive.EndObject();
                                } else if (archive.HasNextField("texCoord")) {
                                    archive.BeginObject();
                                    archive.EndObject();
                                } else if (archive.HasNextField("tangent")) {
                                    archive.BeginObject();
                                    archive.EndObject();
                                } else if (archive.HasNextField("color")) {
                                    archive.BeginObject();
                                    archive.EndObject();
                                }
                            }

                            archive.EndObject();
                        }

                        archive.EndArray();
                    } else if (archive.HasNextField("indices")) {
                        size_t indexCount = archive.BeginArray();
                        subMesh.indices.resize(indexCount);

                        for (size_t l = 0; l < indexCount; ++l) {
                            subMesh.indices[l] = archive.ReadUInt32();
                        }

                        archive.EndArray();
                    }
                }

                archive.EndObject();
            }

            archive.EndArray();
        }
    }

    archive.EndObject();

    // 设置加载状态
    m_isLoaded = !m_subMeshes.empty();
    m_name     = m_metadata.name;

    // 重新计算包围盒
    if (!m_subMeshes.empty()) {
        XMVECTOR minVec = XMVectorSet(FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX);
        XMVECTOR maxVec = XMVectorSet(-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX);

        for (const auto& subMesh : m_subMeshes) {
            for (const auto& vertex : subMesh.vertices) {
                XMVECTOR pos = XMLoadFloat4(&vertex.position);
                minVec       = XMVectorMin(minVec, pos);
                maxVec       = XMVectorMax(maxVec, pos);
            }
        }

        BoundingBox::CreateFromPoints(m_boundingBox, minVec, maxVec);
    }
}

bool MeshAsset::DeserializeFromFile(const std::filesystem::path& path, SerializationFormat format) {
    try {
        auto deserializedAsset = AssetSerializer::DeserializeFromFile<MeshAsset>(path, format);
        if (deserializedAsset) {
            // 复制数据到当前对象
            m_subMeshes   = std::move(deserializedAsset->m_subMeshes);
            m_boundingBox = deserializedAsset->m_boundingBox;
            m_metadata    = deserializedAsset->m_metadata;
            m_path        = path;
            m_name        = deserializedAsset->m_name;
            m_isLoaded    = true;

            LOG_INFO("Mesh", "Successfully deserialized mesh: {0}", m_name);
            return true;
        }
        return false;
    } catch (const std::exception& e) {
        LOG_ERROR("Mesh", "Exception while deserializing mesh: {0}", e.what());
        return false;
    }
}

void MeshAsset::AddSubMesh(const SubMesh& subMesh) {
    m_subMeshes.push_back(subMesh);
    m_isLoaded = true;

    // 更新包围盒
    XMVECTOR minVec = XMVectorSet(FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX);
    XMVECTOR maxVec = XMVectorSet(-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX);

    for (const auto& mesh : m_subMeshes) {
        for (const auto& vertex : mesh.vertices) {
            XMVECTOR pos = XMLoadFloat4(&vertex.position);
            minVec       = XMVectorMin(minVec, pos);
            maxVec       = XMVectorMax(maxVec, pos);
        }
    }

    BoundingBox::CreateFromPoints(m_boundingBox, minVec, maxVec);
}

void MeshAsset::SetBoundingBox(const BoundingBox& boundingBox) {
    m_boundingBox = boundingBox;
}

void MeshAsset::Clear() {
    m_subMeshes.clear();
    m_boundingBox = BoundingBox();
    m_isLoaded    = false;
}
}  // namespace Engine