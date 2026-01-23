#pragma once

#include "../core/Map.h"
#include "../TilemapAsset.h"
#include "../../Component.h"
#include "../../graphic/RenderComponent.h"
#include "../../graphic/interfaces/RenderTypes.h"
#include <memory>
#include <vector>
#include <unordered_map>

namespace PrismaEngine {

namespace Graphic {
class IRenderDevice;
class IBuffer;
class ITexture;
class IPipeline;
class IMaterial;
class ICamera;
}

// ============================================================================
// 瓦片顶点
// ============================================================================

struct TileVertex {
    float x, y;          // 位置
    float u, v;          // 纹理坐标
    float texIndex;      // 纹理数组索引
    float r, g, b, a;    // 颜色 (用于淡入淡出)

    TileVertex() = default;

    TileVertex(float px, float py, float pu, float pv, float ti, float tr, float tg, float tb, float ta)
        : x(px), y(py), u(pu), v(pv), texIndex(ti), r(tr), g(tg), b(tb), a(ta) {}
};

// ============================================================================
// 瓦片变化 (用于动画更新)
// ============================================================================

struct TileChange {
    int x, y;
    uint32_t newGid;
};

// ============================================================================
// 渲染模式
// ============================================================================

enum class TilemapRenderMode {
    Static,      // 静态模式 - 一次性构建所有几何体
    Dynamic,     // 动态模式 - 每帧重建
    Chunked      // 分块模式 - 分块渲染 (推荐用于大地图)
};

// ============================================================================
// 瓦片地图渲染器
// ============================================================================

class TilemapRenderer : public Component {
public:
    TilemapRenderer();
    ~TilemapRenderer() override;

    // Component 接口
    void Initialize() override;
    void Update(float deltaTime) override;
    void Shutdown() override;

    // =====================================================================
    // 设置
    // =====================================================================

    // 设置瓦片地图资源
    void SetTilemap(TilemapAsset* tilemap);
    TilemapAsset* GetTilemap() const { return m_tilemap; }

    // 设置渲染设备
    void SetRenderDevice(Graphic::IRenderDevice* device) { m_device = device; }

    // 设置相机
    void SetCamera(Graphic::ICamera* camera) { m_camera = camera; }
    Graphic::ICamera* GetCamera() const { return m_camera; }

    // 设置渲染模式
    void SetRenderMode(TilemapRenderMode mode) { m_renderMode = mode; m_geometryDirty = true; }
    TilemapRenderMode GetRenderMode() const { return m_renderMode; }

    // 设置块大小 (仅 Chunked 模式)
    void SetChunkSize(int size) { m_chunkSize = size; m_geometryDirty = true; }
    int GetChunkSize() const { return m_chunkSize; }

    // =====================================================================
    // 层控制
    // =====================================================================

    // 设置层可见性
    void SetLayerVisibility(size_t layerIndex, bool visible);
    bool GetLayerVisibility(size_t layerIndex) const;

    // 设置层透明度
    void SetLayerOpacity(size_t layerIndex, float opacity);
    float GetLayerOpacity(size_t layerIndex) const;

    // =====================================================================
    // 动态更新
    // =====================================================================

    // 设置指定位置的瓦片
    void SetTile(int x, int y, uint32_t gid);
    uint32_t GetTile(int x, int y) const;

    // 批量更新瓦片
    void SetTiles(const std::vector<TileChange>& changes);

    // 刷新几何体 (在修改瓦片后调用)
    void RefreshGeometry();

    // =====================================================================
    // 渲染
    // =====================================================================

    // 渲染瓦片地图
    void Render(Graphic::RenderCommandContext* context);

    // =====================================================================
    // 动画瓦片
    // =====================================================================

    // 启用/禁用动画瓦片
    void SetAnimatedTilesEnabled(bool enabled) { m_animatedTilesEnabled = enabled; }
    bool AreAnimatedTilesEnabled() const { return m_animatedTilesEnabled; }

    // 更新动画瓦片 (在 Update 中自动调用)
    void UpdateAnimatedTiles(float deltaTime);

private:
    // =====================================================================
    // 几何构建
    // =====================================================================

    // 构建所有几何体
    void BuildGeometry();

    // 构建指定层的几何体
    void BuildLayerGeometry(TileLayer* layer, std::vector<TileVertex>& vertices, std::vector<uint32_t>& indices);

    // 构建分块几何体
    void BuildChunkGeometry();

    // 构建指定块的几何体
    void BuildChunkGeometry(int chunkX, int chunkY);

    // =====================================================================
    // 纹理管理
    // =====================================================================

    // 加载图块集纹理
    bool LoadTilesetTextures();

    // 获取图块集的纹理索引
    int GetTilesetTextureIndex(const Tileset* tileset) const;

    // =====================================================================
    // 动画管理
    // =====================================================================

    // 注册动画瓦片
    void RegisterAnimatedTiles();

    // 更新动画帧
    void UpdateAnimationFrame(int x, int y, const Tile* tile, uint32_t& currentGid);

    // =====================================================================
    // 辅助方法
    // =====================================================================

    // 计算块的 UV 坐标
    void GetTileUV(const Tileset* tileset, int localId, float& u0, float& v0, float& u1, float& v1) const;

    // 获取瓦片的世界位置
    void GetTilePosition(int x, int y, float& worldX, float& worldY) const;

    // 创建材质
    void CreateMaterial();

private:
    // 瓦片地图资源
    TilemapAsset* m_tilemap = nullptr;

    // 渲染接口
    Graphic::IRenderDevice* m_device = nullptr;
    Graphic::ICamera* m_camera = nullptr;

    // 材质
    std::shared_ptr<Graphic::Material> m_material;

    // 图块集纹理 (最多 16 个)
    std::vector<std::shared_ptr<Graphic::ITexture>> m_tilesetTextures;
    std::unordered_map<const Tileset*, int> m_tilesetToTextureIndex;

    // 几何体数据
    std::vector<TileVertex> m_vertices;
    std::vector<uint32_t> m_indices;
    std::shared_ptr<Graphic::IBuffer> m_vertexBuffer;
    std::shared_ptr<Graphic::IBuffer> m_indexBuffer;
    uint32_t m_vertexCount = 0;
    uint32_t m_indexCount = 0;

    // 分块渲染数据
    static constexpr int MAX_CHUNKS = 256;
    struct Chunk {
        std::vector<TileVertex> vertices;
        std::vector<uint32_t> indices;
        std::shared_ptr<Graphic::IBuffer> vertexBuffer;
        std::shared_ptr<Graphic::IBuffer> indexBuffer;
        uint32_t vertexCount = 0;
        uint32_t indexCount = 0;
        bool dirty = true;
        bool hasData = false;
    };
    Chunk m_chunks[MAX_CHUNKS];
    int m_chunksX = 0;
    int m_chunksY = 0;

    // 层状态
    struct LayerState {
        bool visible = true;
        float opacity = 1.0f;
    };
    std::vector<LayerState> m_layerStates;

    // 渲染模式
    TilemapRenderMode m_renderMode = TilemapRenderMode::Chunked;
    int m_chunkSize = 32;

    // 几何体状态
    bool m_geometryDirty = true;
    bool m_materialDirty = true;

    // 动画瓦片
    bool m_animatedTilesEnabled = true;
    struct AnimatedTileInfo {
        int x, y;
        const Tile* tile;
        float frameTimer;
        size_t currentFrame;
        uint32_t baseGid;
    };
    std::vector<AnimatedTileInfo> m_animatedTiles;
};

} // namespace PrismaEngine
