#include "MeshAsset.h"
#include "AssetSerializer.h"
#include "Logger.h"

#include <fstream>
#include <sstream>
#include <algorithm>

namespace PrismaEngine {

using namespace Serialization;

bool MeshAsset::Load(const std::filesystem::path& path) {
    try {
        if (!std::filesystem::exists(path)) {
            LOG_ERROR("Mesh", "Mesh file does not exist: {0}", path.string());
            return false;
        }

        SubMesh triangle;
        triangle.name          = "Triangle";
        triangle.materialIndex = 0;

        triangle.vertices.resize(3);
        triangle.vertices[0].position = {0.0f, 0.5f, 0.0f, 1.0f};
        triangle.vertices[1].position = {-0.5f, -0.5f, 0.0f, 1.0f};
        triangle.vertices[2].position = {0.5f, -0.5f, 0.0f, 1.0f};

        triangle.indices = {0, 1, 2};
        m_subMeshes.push_back(triangle);

        m_path                = path;
        m_name                = path.filename().string();
        m_metadata.sourcePath = path;
        m_metadata.name       = m_name;

        m_isLoaded = true;
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Mesh", "Exception while loading mesh: {0}", e.what());
        return false;
    }
}

void MeshAsset::Unload() {
    m_subMeshes.clear();
    m_isLoaded    = false;
}

void MeshAsset::Serialize(OutputArchive& archive) const {
    archive.BeginObject("MeshAsset");
    archive("metadata", m_metadata);
    
    uint32_t count = static_cast<uint32_t>(m_subMeshes.size());
    archive.BeginArray("subMeshes", count);
    for (const auto& subMesh : m_subMeshes) {
        archive.BeginObject("SubMesh");
        archive("name", subMesh.name);
        archive("materialIndex", subMesh.materialIndex);
        archive.EndObject();
    }
    archive.EndArray();
    archive.EndObject();
}

void MeshAsset::Deserialize(InputArchive& archive) {
    archive.BeginObject("MeshAsset");
    archive("metadata", m_metadata);

    uint32_t count = 0;
    archive.BeginArray("subMeshes", count);
    m_subMeshes.resize(count);
    for (uint32_t i = 0; i < count; ++i) {
        archive.BeginObject("SubMesh");
        archive("name", m_subMeshes[i].name);
        archive("materialIndex", m_subMeshes[i].materialIndex);
        archive.EndObject();
    }
    archive.EndArray();
    archive.EndObject();

    m_isLoaded = true;
    m_name = m_metadata.name;
}

bool MeshAsset::DeserializeFromFile(const std::filesystem::path& path, SerializationFormat format) {
    auto deserializedAsset = AssetSerializer::DeserializeFromFile<MeshAsset>(path, format);
    if (deserializedAsset) {
        m_subMeshes = std::move(deserializedAsset->m_subMeshes);
        m_metadata = deserializedAsset->m_metadata;
        m_path = path;
        m_name = deserializedAsset->m_name;
        m_isLoaded = true;
        return true;
    }
    return false;
}

void MeshAsset::AddSubMesh(const SubMesh& subMesh) {
    m_subMeshes.push_back(subMesh);
    m_isLoaded = true;
}

void MeshAsset::SetBoundingBox(const BoundingBox& boundingBox) {
    m_boundingBox = boundingBox;
}

void MeshAsset::Clear() {
    m_subMeshes.clear();
    m_isLoaded = false;
}

} // namespace PrismaEngine
