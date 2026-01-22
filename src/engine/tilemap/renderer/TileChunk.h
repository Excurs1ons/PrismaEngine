#pragma once

#include "../core/Map.h"
#include "TilemapRenderer.h"
#include <vector>
#include <memory>

namespace PrismaEngine {

namespace Graphic {
class IBuffer;
}

// ============================================================================
// 瓦块 - 用于分块渲染优化
// ============================================================================

class TileChunk {
public:
    TileChunk() = default;
    ~TileChunk() = default;

    // 初始化块
    void Initialize(
        int chunkX, int chunkY,
        int chunkSize,
        int tileWidth, int tileHeight
    );

    // 构建几何体
    void BuildGeometry(
        const TileMap& map,
        const std::vector<TileLayer*>& layers
    );

    // 重建几何体 (标记为脏)
    void MarkDirty() { m_dirty = true; }
    bool IsDirty() const { return m_dirty; }

    // 获取渲染数据
    const std::vector<float>& GetVertices() const { return m_vertices; }
    const std::vector<uint32_t>& GetIndices() const { return m_indices; }
    size_t GetVertexCount() const { return m_vertexCount; }
    size_t GetIndexCount() const { return m_indexCount; }

    // GPU 缓冲区
    void SetVertexBuffer(std::shared_ptr<Graphic::IBuffer> buffer) { m_vertexBuffer = buffer; }
    std::shared_ptr<Graphic::IBuffer> GetVertexBuffer() const { return m_vertexBuffer; }

    void SetIndexBuffer(std::shared_ptr<Graphic::IBuffer> buffer) { m_indexBuffer = buffer; }
    std::shared_ptr<Graphic::IBuffer> GetIndexBuffer() const { return m_indexBuffer; }

    // 检查是否有数据
    bool HasData() const { return m_hasData; }

    // 获取块的世界边界
    void GetBounds(float& minX, float& minY, float& maxX, float& maxY) const;

    // 检查块是否在视锥内
    bool IsVisible(
        float viewMinX, float viewMinY,
        float viewMaxX, float viewMaxY
    ) const;

private:
    // 块位置
    int m_chunkX = 0;
    int m_chunkY = 0;
    int m_chunkSize = 0;
    int m_tileWidth = 0;
    int m_tileHeight = 0;

    // 几何体数据
    std::vector<float> m_vertices;
    std::vector<uint32_t> m_indices;
    size_t m_vertexCount = 0;
    size_t m_indexCount = 0;

    // GPU 缓冲区
    std::shared_ptr<Graphic::IBuffer> m_vertexBuffer;
    std::shared_ptr<Graphic::IBuffer> m_indexBuffer;

    // 状态
    bool m_dirty = true;
    bool m_hasData = false;
};

// ============================================================================
// 块管理器 - 管理所有块
// ============================================================================

class TileChunkManager {
public:
    TileChunkManager();
    ~TileChunkManager();

    // 初始化块管理器
    void Initialize(
        const TileMap& map,
        int chunkSize = 32
    );

    // 重建所有块
    void RebuildAll(const TileMap& map);

    // 获取指定位置的块
    TileChunk* GetChunk(int chunkX, int chunkY);

    // 获取块坐标
    void GetChunkCoord(
        float worldX, float worldY,
        int& chunkX, int& chunkY
    ) const;

    // 获取可见块
    std::vector<TileChunk*> GetVisibleChunks(
        float viewMinX, float viewMinY,
        float viewMaxX, float viewMaxY
    );

    // 更新脏块
    void UpdateDirtyChunks(
        const TileMap& map,
        Graphic::IRenderDevice* device
    );

    // 获取块数量
    int GetChunkCountX() const { return m_chunksX; }
    int GetChunkCountY() const { return m_chunksY; }
    int GetTotalChunkCount() const { return m_totalChunks; }

private:
    std::vector<TileChunk> m_chunks;
    int m_chunksX = 0;
    int m_chunksY = 0;
    int m_totalChunks = 0;
    int m_chunkSize = 32;
};

} // namespace PrismaEngine
