#pragma once

#include "../core/Map.h"
#include "TilemapRenderer.h"
#include <vector>
#include <memory>

namespace PrismaEngine {

// ============================================================================
// 几何体构建器 - 用于构建瓦片地图的渲染几何体
// ============================================================================

class TileGeometryBuilder {
public:
    // 构建整个地图的几何体
    static void BuildMapGeometry(
        const TileMap& map,
        std::vector<TileVertex>& outVertices,
        std::vector<uint32_t>& outIndices
    );

    // 构建单个层的几何体
    static void BuildLayerGeometry(
        const TileMap& map,
        const TileLayer& layer,
        std::vector<TileVertex>& outVertices,
        std::vector<uint32_t>& outIndices,
        float opacity = 1.0f,
        const std::string& tint = ""
    );

    // 构建指定区域的几何体 (用于分块渲染)
    static void BuildRegionGeometry(
        const TileMap& map,
        int startX, int startY,
        int width, int height,
        std::vector<TileVertex>& outVertices,
        std::vector<uint32_t>& outIndices
    );

    // 计算几何体统计信息
    struct GeometryStats {
        size_t vertexCount = 0;
        size_t indexCount = 0;
        size_t triangleCount = 0;
    };

    static GeometryStats EstimateGeometry(const TileMap& map);
    static GeometryStats EstimateLayer(const TileLayer& layer);

    // 坐标转换工具
    static void TileToWorld(
        int tileX, int tileY,
        int tileWidth, int tileHeight,
        Orientation orientation,
        float& worldX, float& worldY
    );

    static void WorldToTile(
        float worldX, float worldY,
        int tileWidth, int tileHeight,
        Orientation orientation,
        int& tileX, int& tileY
    );

private:
    // 添加单个瓦片的四边形
    static void AddTileQuad(
        float x, float y, float width, float height,
        float u0, float v0, float u1, float v1,
        float texIndex,
        float r, float g, float b, float a,
        bool flipH, bool flipV, bool flipD,
        std::vector<TileVertex>& vertices,
        std::vector<uint32_t>& indices,
        uint32_t& vertexOffset
    );
};

} // namespace PrismaEngine
