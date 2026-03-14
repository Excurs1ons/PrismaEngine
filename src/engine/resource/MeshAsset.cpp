#include "MeshAsset.h"
#include "AssetSerializer.h"
#include "Logger.h"

#include <fstream>
#include <sstream>
#include <algorithm>

namespace Prisma {

using namespace Serialization;

bool MeshAsset::Load(const std::filesystem::path& path) {
    try {
        if (!std::filesystem::exists(path)) {
            LOG_ERROR("MeshAsset", "Mesh file does not exist: {0}", path.string());
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

        Path                = path;
        Name                = path.filename().string();
        m_Metadata.sourcePath = path;
        m_Metadata.name       = Name;

        SetLoaded(true);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("MeshAsset", "Exception while loading mesh: {0}", e.what());
        return false;
    }
}

void MeshAsset::Unload() {
    Asset::Unload();
    m_subMeshes.clear();
}

void MeshAsset::Serialize(OutputArchive& archive) const {
    Asset::Serialize(archive);
    
    uint32_t count = static_cast<uint32_t>(m_subMeshes.size());
    archive.BeginArray("subMeshes", count);
    for (const auto& subMesh : m_subMeshes) {
        archive.BeginObject("SubMesh");
        archive("name", subMesh.name);
        archive("materialIndex", subMesh.materialIndex);
        archive.EndObject();
    }
    archive.EndArray();
}

void MeshAsset::Deserialize(InputArchive& archive) {
    Asset::Deserialize(archive);

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

    SetLoaded(true);
}

void MeshAsset::AddSubMesh(const SubMesh& subMesh) {
    m_subMeshes.push_back(subMesh);
    SetLoaded(true);
}

void MeshAsset::SetBoundingBox(const Graphic::BoundingBox& boundingBox) {
    m_boundingBox = boundingBox;
}

void MeshAsset::Clear() {
    m_subMeshes.clear();
    SetLoaded(false);
}

} // namespace Prisma
