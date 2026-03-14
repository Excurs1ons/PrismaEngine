#include "MeshAsset.h"
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

        SetPath(path);
        
        SetLoaded(true);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("MeshAsset", "Exception while loading mesh: {0}", e.what());
        return false;
    }
}

void MeshAsset::Unload() {
    m_subMeshes.clear();
    SetLoaded(false);
}

void MeshAsset::Serialize(OutputArchive& archive) const {
    Asset::Serialize(archive);
    
    // Simplistic serialization for now
    archive.Write("subMeshCount", static_cast<uint32_t>(m_subMeshes.size()));
}

void MeshAsset::Deserialize(InputArchive& archive) {
    Asset::Deserialize(archive);

    uint32_t count = 0;
    archive.Read("subMeshCount", count);
    m_subMeshes.resize(count);

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
