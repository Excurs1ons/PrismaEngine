#include "water/WaterMesh.h"
#include <algorithm>
#include <cmath>

namespace Prisma::Water {

void WaterMesh::Generate(const Vector3& cameraPosition,
                          const Vector2& tileSize,
                          uint32_t baseSegments,
                          uint32_t farSegments,
                          float lodDistance)
{
    m_Vertices.clear();
    m_Indices.clear();

    float cameraDist = std::abs(cameraPosition.y);
    float t = std::clamp(cameraDist / lodDistance, 0.0f, 1.0f);
    uint32_t segments = static_cast<uint32_t>(
        std::round(static_cast<float>(baseSegments) * (1.0f - t) +
                   static_cast<float>(farSegments) * t));
    segments = std::max(segments, 4u);

    float halfW = tileSize.x * 0.5f;
    float halfD = tileSize.y * 0.5f;

    float centerX = std::floor(cameraPosition.x / tileSize.x) * tileSize.x;
    float centerZ = std::floor(cameraPosition.z / tileSize.y) * tileSize.y;

    float startX = centerX - halfW;
    float startZ = centerZ - halfD;
    float stepX = tileSize.x / static_cast<float>(segments);
    float stepZ = tileSize.y / static_cast<float>(segments);

    uint32_t vertsPerSide = segments + 1;
    uint32_t totalVerts = vertsPerSide * vertsPerSide;
    m_Vertices.resize(totalVerts);

    for (uint32_t z = 0; z < vertsPerSide; ++z) {
        for (uint32_t x = 0; x < vertsPerSide; ++x) {
            uint32_t idx = z * vertsPerSide + x;

            float wx = startX + static_cast<float>(x) * stepX;
            float wz = startZ + static_cast<float>(z) * stepZ;

            WaterVertex& vert = m_Vertices[idx];
            vert.position = Vector3(wx, 0.0f, wz);
            vert.normal = Vector3(0.0f, 1.0f, 0.0f);
            vert.uv = Vector2(
                static_cast<float>(x) / static_cast<float>(segments),
                static_cast<float>(z) / static_cast<float>(segments));
            vert.tangent = Vector3(1.0f, 0.0f, 0.0f);
        }
    }

    uint32_t totalIndices = segments * segments * 6;
    m_Indices.resize(totalIndices);

    uint32_t idx = 0;
    for (uint32_t z = 0; z < segments; ++z) {
        for (uint32_t x = 0; x < segments; ++x) {
            uint32_t i0 = z * vertsPerSide + x;
            uint32_t i1 = z * vertsPerSide + (x + 1);
            uint32_t i2 = (z + 1) * vertsPerSide + x;
            uint32_t i3 = (z + 1) * vertsPerSide + (x + 1);

            m_Indices[idx++] = i0;
            m_Indices[idx++] = i1;
            m_Indices[idx++] = i2;

            m_Indices[idx++] = i1;
            m_Indices[idx++] = i3;
            m_Indices[idx++] = i2;
        }
    }

    m_BoundsMin = Vector3(-halfW, -0.5f, -halfD);
    m_BoundsMax = Vector3(halfW, 0.5f, halfD);
}

void WaterMesh::Clear()
{
    m_Vertices.clear();
    m_Indices.clear();
    m_Vertices.shrink_to_fit();
    m_Indices.shrink_to_fit();
}

} // namespace Prisma::Water
