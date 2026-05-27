#pragma once

#include "math/MathTypes.h"
#include "Export.h"
#include <vector>
#include <cstdint>

namespace Prisma::Water {

/**
 * Water vertex structure
 * position: world position
 * normal: surface normal
 * uv: texture coordinates
 * tangent: tangent vector (for normal mapping / wave displacement)
 */
struct WaterVertex {
    Vector3 position;
    Vector3 normal;
    Vector2 uv;
    Vector3 tangent;
};

/**
 * Water mesh index
 */
using WaterIndex = uint32_t;

/**
 * @brief WaterMesh - tiled grid plane with distance-based tessellation
 * 
 * Generates a tessellated grid mesh for water rendering.
 * Near the camera, more segments are used; far away, fewer segments.
 * This avoids geometry LOD popping since water hides seams well.
 */
class ENGINE_API WaterMesh {
public:
    WaterMesh() = default;
    ~WaterMesh() = default;

    /**
     * @brief Generate water mesh for given camera position
     * @param cameraPosition  camera world position
     * @param tileSize       water surface world size (x, z)
     * @param baseSegments   segment count at full detail (near)
     * @param farSegments    segment count at distance (far)
     * @param lodDistance    distance threshold for LOD transition
     */
    void Generate(const Vector3& cameraPosition,
                  const Vector2& tileSize,
                  uint32_t baseSegments,
                  uint32_t farSegments,
                  float lodDistance);

    /**
     * @brief Clear all mesh data
     */
    void Clear();

    // Accessors
    const std::vector<WaterVertex>& GetVertices() const { return m_Vertices; }
    const std::vector<WaterIndex>& GetIndices() const { return m_Indices; }
    size_t GetVertexCount() const { return m_Vertices.size(); }
    size_t GetIndexCount() const { return m_Indices.size(); }
    size_t GetTriangleCount() const { return m_Indices.size() / 3; }
    bool IsValid() const { return !m_Vertices.empty() && !m_Indices.empty(); }

    // Bounds for culling
    Vector3 GetBoundsMin() const { return m_BoundsMin; }
    Vector3 GetBoundsMax() const { return m_BoundsMax; }

private:
    std::vector<WaterVertex> m_Vertices;
    std::vector<WaterIndex> m_Indices;
    Vector3 m_BoundsMin = Vector3(0.0f);
    Vector3 m_BoundsMax = Vector3(0.0f);
};

} // namespace Prisma::Water
