#include "Mesh.h"
#include "Logger.h"
#include <fstream>
#include <limits>
#include <sstream>

namespace Prisma::Graphic {

bool Mesh::Load(const std::filesystem::path& path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        LOG_ERROR("Mesh", "无法打开网格文件: {0}", path.string());
        return false;
    }

    std::vector<Prisma::Vector3> positions;
    std::vector<uint32_t> indices;
    std::string line;

    auto resolveIndex = [](int rawIndex, size_t count) -> uint32_t {
        if (rawIndex > 0) {
            return static_cast<uint32_t>(rawIndex - 1);
        }
        if (rawIndex < 0) {
            return static_cast<uint32_t>(static_cast<int>(count) + rawIndex);
        }
        return std::numeric_limits<uint32_t>::max();
    };

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::istringstream iss(line);
        std::string keyword;
        iss >> keyword;

        if (keyword == "v") {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
            if (iss >> x >> y >> z) {
                positions.emplace_back(x, y, z);
            }
            continue;
        }

        if (keyword != "f") {
            continue;
        }

        std::vector<uint32_t> faceIndices;
        std::string token;
        while (iss >> token) {
            const size_t slash = token.find('/');
            const std::string vertexToken = token.substr(0, slash);
            const int rawIndex = std::stoi(vertexToken);
            const uint32_t resolvedIndex = resolveIndex(rawIndex, positions.size());
            if (resolvedIndex == std::numeric_limits<uint32_t>::max() || resolvedIndex >= positions.size()) {
                LOG_WARNING("Mesh", "跳过网格 {1} 中的无效面索引 {0}", rawIndex, path.string());
                faceIndices.clear();
                break;
            }
            faceIndices.push_back(resolvedIndex);
        }

        if (faceIndices.size() < 3) {
            continue;
        }

        for (size_t i = 1; i + 1 < faceIndices.size(); ++i) {
            indices.push_back(faceIndices[0]);
            indices.push_back(faceIndices[i]);
            indices.push_back(faceIndices[i + 1]);
        }
    }

    if (positions.empty() || indices.empty()) {
        LOG_ERROR("Mesh", "网格文件不包含可用的几何数据: {0}", path.string());
        return false;
    }

    m_SubMeshes.clear();
    m_BoundingBox = BoundingBox(
        Prisma::Vector3(std::numeric_limits<float>::max()),
        Prisma::Vector3(std::numeric_limits<float>::lowest()));
    for (const auto& position : positions) {
        m_BoundingBox.Encapsulate(position);
    }

    SubMeshBuffer subMesh;
    subMesh.name = path.stem().string();
    subMesh.materialIndex = 0;
    subMesh.baseVertex = 0;
    subMesh.baseIndex = 0;
    subMesh.indexCount = static_cast<uint32_t>(indices.size());
    subMesh.vertexCount = static_cast<uint32_t>(positions.size());
    subMesh.use16BitIndices = (positions.size() <= static_cast<size_t>(std::numeric_limits<uint16_t>::max()));
    m_SubMeshes.push_back(std::move(subMesh));

    SetPath(path);
    SetName(path.stem().string());
    m_IsLoaded = true;
    return true;
}

void Mesh::Unload()
{
    m_SubMeshes.clear();
    m_BoundingBox = BoundingBox();
    m_IsLoaded = false;
}

} // namespace Prisma::Graphic
