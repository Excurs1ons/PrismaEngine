#include "TilemapRenderer.h"
#include "../../graphic/interfaces/IResourceManager.h"
#include "../../graphic/Material.h"
#include "../../graphic/RenderCommandContext.h"
#include "../../graphic/Shader.h"
#include "../../graphic/ICamera.h"
#include <algorithm>
#include <cmath>

namespace PrismaEngine {

// ============================================================================
// 构造/析构
// ============================================================================

TilemapRenderer::TilemapRenderer() = default;

TilemapRenderer::~TilemapRenderer() {
    Shutdown();
}

// ============================================================================
// Component 接口
// ============================================================================

void TilemapRenderer::Initialize() {
    // 材质会在首次渲染时创建
}

void TilemapRenderer::Update(float deltaTime) {
    if (!m_tilemap || !m_tilemap->IsLoaded()) {
        return;
    }

    // 更新动画瓦片
    if (m_animatedTilesEnabled) {
        UpdateAnimatedTiles(deltaTime);
    }

    // 如果几何体脏，重建
    if (m_geometryDirty) {
        BuildGeometry();
    }
}

void TilemapRenderer::Shutdown() {
    m_tilemap = nullptr;
    m_material.reset();
    m_tilesetTextures.clear();
    m_tilesetToTextureIndex.clear();
    m_vertexBuffer.reset();
    m_indexBuffer.reset();
    m_animatedTiles.clear();
}

// ============================================================================
// 设置
// ============================================================================

void TilemapRenderer::SetTilemap(TilemapAsset* tilemap) {
    m_tilemap = tilemap;

    if (m_tilemap && m_tilemap->IsLoaded()) {
        // 加载图块集纹理
        LoadTilesetTextures();

        // 注册动画瓦片
        if (m_animatedTilesEnabled) {
            RegisterAnimatedTiles();
        }

        // 计算块数量
        if (m_tilemap->GetWidth() > 0 && m_tilemap->GetHeight() > 0) {
            m_chunksX = (m_tilemap->GetWidth() + m_chunkSize - 1) / m_chunkSize;
            m_chunksY = (m_tilemap->GetHeight() + m_chunkSize - 1) / m_chunkSize;
        }

        // 初始化层状态
        m_layerStates.clear();
        // TODO: 从地图获取层数
    }

    m_geometryDirty = true;
}

// ============================================================================
// 层控制
// ============================================================================

void TilemapRenderer::SetLayerVisibility(size_t layerIndex, bool visible) {
    if (layerIndex >= m_layerStates.size()) {
        m_layerStates.resize(layerIndex + 1);
    }
    m_layerStates[layerIndex].visible = visible;
    m_geometryDirty = true;
}

bool TilemapRenderer::GetLayerVisibility(size_t layerIndex) const {
    if (layerIndex >= m_layerStates.size()) {
        return true;
    }
    return m_layerStates[layerIndex].visible;
}

void TilemapRenderer::SetLayerOpacity(size_t layerIndex, float opacity) {
    if (layerIndex >= m_layerStates.size()) {
        m_layerStates.resize(layerIndex + 1);
    }
    m_layerStates[layerIndex].opacity = std::clamp(opacity, 0.0f, 1.0f);
    m_geometryDirty = true;
}

float TilemapRenderer::GetLayerOpacity(size_t layerIndex) const {
    if (layerIndex >= m_layerStates.size()) {
        return 1.0f;
    }
    return m_layerStates[layerIndex].opacity;
}

// ============================================================================
// 动态更新
// ============================================================================

void TilemapRenderer::SetTile(int x, int y, uint32_t gid) {
    if (!m_tilemap || !m_tilemap->IsLoaded()) {
        return;
    }

    const TileMap* map = m_tilemap->GetMap();
    if (!map) return;

    // TODO: 更新地图数据
    // map->SetTile(x, y, gid);

    m_geometryDirty = true;
}

uint32_t TilemapRenderer::GetTile(int x, int y) const {
    if (!m_tilemap || !m_tilemap->IsLoaded()) {
        return 0;
    }

    const TileMap* map = m_tilemap->GetMap();
    if (!map) return 0;

    // TODO: 从地图获取瓦片
    // return map->GetTile(x, y);
    return 0;
}

void TilemapRenderer::SetTiles(const std::vector<TileChange>& changes) {
    for (const auto& change : changes) {
        SetTile(change.x, change.y, change.newGid);
    }
}

void TilemapRenderer::RefreshGeometry() {
    m_geometryDirty = true;
}

// ============================================================================
// 渲染
// ============================================================================

void TilemapRenderer::Render(Graphic::RenderCommandContext* context) {
    if (!m_tilemap || !m_tilemap->IsLoaded()) {
        return;
    }

    // 确保材质已创建
    if (m_materialDirty || !m_material) {
        CreateMaterial();
    }

    // TODO: 实现实际的渲染命令
    // 这里需要根据具体的渲染接口实现
}

// ============================================================================
// 动画瓦片
// ============================================================================

void TilemapRenderer::UpdateAnimatedTiles(float deltaTime) {
    if (!m_tilemap || !m_tilemap->IsLoaded()) {
        return;
    }

    for (auto& animTile : m_animatedTiles) {
        if (!animTile.tile || animTile.tile->animation.empty()) {
            continue;
        }

        // 更新计时器
        animTile.frameTimer += deltaTime * 1000.0f; // 转换为毫秒

        const Frame& currentFrame = animTile.tile->animation[animTile.currentFrame];
        if (animTile.frameTimer >= currentFrame.duration) {
            animTile.frameTimer = 0.0f;
            animTile.currentFrame = (animTile.currentFrame + 1) % animTile.tile->animation.size();

            // 更新瓦片
            uint32_t newGid = animTile.baseGid + animTile.tile->animation[animTile.currentFrame].tileId;
            SetTile(animTile.x, animTile.y, newGid);
        }
    }
}

// ============================================================================
// 几何构建
// ============================================================================

void TilemapRenderer::BuildGeometry() {
    if (!m_tilemap || !m_tilemap->IsLoaded()) {
        return;
    }

    switch (m_renderMode) {
        case TilemapRenderMode::Static:
        case TilemapRenderMode::Dynamic:
            // 构建完整几何体
            m_vertices.clear();
            m_indices.clear();
            m_vertexCount = 0;
            m_indexCount = 0;

            for (auto* layer : m_tilemap->GetTileLayers()) {
                if (layer) {
                    BuildLayerGeometry(layer, m_vertices, m_indices);
                }
            }
            break;

        case TilemapRenderMode::Chunked:
            BuildChunkGeometry();
            break;
    }

    m_geometryDirty = false;
}

void TilemapRenderer::BuildLayerGeometry(
    TileLayer* layer,
    std::vector<TileVertex>& vertices,
    std::vector<uint32_t>& indices
) {
    if (!layer || !m_tilemap || !m_tilemap->IsLoaded()) {
        return;
    }

    const TileMap* map = m_tilemap->GetMap();
    if (!map) return;

    float layerOpacity = layer->opacity;
    bool layerVisible = layer->visible;

    if (!layerVisible || layerOpacity <= 0.0f) {
        return;
    }

    // 检查层状态
    size_t layerIndex = 0; // TODO: 获取正确的层索引
    if (layerIndex < m_layerStates.size()) {
        if (!m_layerStates[layerIndex].visible) return;
        layerOpacity *= m_layerStates[layerIndex].opacity;
    }

    if (layerOpacity <= 0.0f) return;

    int mapWidth = map->width;
    int mapHeight = map->height;
    int tileWidth = map->tileWidth;
    int tileHeight = map->tileHeight;

    uint32_t vertexOffset = static_cast<uint32_t>(vertices.size());

    for (int y = 0; y < mapHeight; ++y) {
        for (int x = 0; x < mapWidth; ++x) {
            uint32_t gid = layer->tileData.GetGid(x, y);
            uint32_t pureGid = GIDHelper::GetPureGid(gid);

            if (pureGid == 0) continue;

            // 查找图块集
            const Tileset* tileset = map->FindTilesetByGid(gid);
            if (!tileset) continue;

            int textureIndex = GetTilesetTextureIndex(tileset);
            if (textureIndex < 0) continue;

            int localId = static_cast<int>(pureGid) - tileset->firstGid;

            // 获取 UV 坐标
            float u0, v0, u1, v1;
            GetTileUV(tileset, localId, u0, v0, u1, v1);

            // 计算世界位置
            float worldX, worldY;
            GetTilePosition(x, y, worldX, worldY);

            // 颜色 (包含透明度)
            float r = 1.0f, g = 1.0f, b = 1.0f, a = layerOpacity;

            // 检查翻转标志
            bool flipH = GIDHelper::IsHorizontallyFlipped(gid);
            bool flipV = GIDHelper::IsVerticallyFlipped(gid);
            bool flipD = GIDHelper::IsDiagonallyFlipped(gid);

            // 添加 4 个顶点 (两个三角形)
            TileVertex v0, v1, v2, v3;

            if (flipD) {
                // 对角翻转 (旋转)
                v0 = TileVertex(worldX, worldY, flipH ? u1 : u0, flipV ? v1 : v0, textureIndex, r, g, b, a);
                v1 = TileVertex(worldX, worldY + tileHeight, flipH ? u1 : u0, flipV ? v0 : v1, textureIndex, r, g, b, a);
                v2 = TileVertex(worldX + tileWidth, worldY + tileHeight, flipH ? u0 : u1, flipV ? v0 : v1, textureIndex, r, g, b, a);
                v3 = TileVertex(worldX + tileWidth, worldY, flipH ? u0 : u1, flipV ? v1 : v0, textureIndex, r, g, b, a);
            } else {
                v0 = TileVertex(worldX, worldY, flipH ? u1 : u0, flipV ? v1 : v0, textureIndex, r, g, b, a);
                v1 = TileVertex(worldX, worldY + tileHeight, flipH ? u1 : u0, flipV ? v0 : v1, textureIndex, r, g, b, a);
                v2 = TileVertex(worldX + tileWidth, worldY + tileHeight, flipH ? u0 : u1, flipV ? v0 : v1, textureIndex, r, g, b, a);
                v3 = TileVertex(worldX + tileWidth, worldY, flipH ? u0 : u1, flipV ? v1 : v0, textureIndex, r, g, b, a);
            }

            vertices.insert(vertices.end(), {v0, v1, v2, v3});

            // 添加索引
            uint32_t i0 = vertexOffset + 0;
            uint32_t i1 = vertexOffset + 1;
            uint32_t i2 = vertexOffset + 2;
            uint32_t i3 = vertexOffset + 3;

            indices.insert(indices.end(), {i0, i1, i2, i0, i2, i3});

            vertexOffset += 4;
        }
    }
}

void TilemapRenderer::BuildChunkGeometry() {
    if (!m_tilemap || !m_tilemap->IsLoaded()) {
        return;
    }

    const TileMap* map = m_tilemap->GetMap();
    if (!map) return;

    int mapWidth = map->width;
    int mapHeight = map->height;

    m_chunksX = (mapWidth + m_chunkSize - 1) / m_chunkSize;
    m_chunksY = (mapHeight + m_chunkSize - 1) / m_chunkSize;

    for (int chunkY = 0; chunkY < m_chunksY; ++chunkY) {
        for (int chunkX = 0; chunkX < m_chunksX; ++chunkX) {
            BuildChunkGeometry(chunkX, chunkY);
        }
    }
}

void TilemapRenderer::BuildChunkGeometry(int chunkX, int chunkY) {
    int chunkIndex = chunkY * m_chunksX + chunkX;
    if (chunkIndex >= MAX_CHUNKS) return;

    Chunk& chunk = m_chunks[chunkIndex];
    chunk.vertices.clear();
    chunk.indices.clear();
    chunk.dirty = false;
    chunk.hasData = false;

    if (!m_tilemap || !m_tilemap->IsLoaded()) {
        return;
    }

    const TileMap* map = m_tilemap->GetMap();
    if (!map) return;

    int startX = chunkX * m_chunkSize;
    int startY = chunkY * m_chunkSize;
    int endX = std::min(startX + m_chunkSize, map->width);
    int endY = std::min(startY + m_chunkSize, map->height);

    int tileWidth = map->tileWidth;
    int tileHeight = map->tileHeight;

    uint32_t vertexOffset = 0;

    for (auto* layer : m_tilemap->GetTileLayers()) {
        if (!layer || !layer->visible) continue;

        for (int y = startY; y < endY; ++y) {
            for (int x = startX; x < endX; ++x) {
                uint32_t gid = layer->tileData.GetGid(x, y);
                uint32_t pureGid = GIDHelper::GetPureGid(gid);

                if (pureGid == 0) continue;

                const Tileset* tileset = map->FindTilesetByGid(gid);
                if (!tileset) continue;

                int textureIndex = GetTilesetTextureIndex(tileset);
                if (textureIndex < 0) continue;

                int localId = static_cast<int>(pureGid) - tileset->firstGid;

                float u0, v0, u1, v1;
                GetTileUV(tileset, localId, u0, v0, u1, v1);

                float worldX = static_cast<float>(x * tileWidth);
                float worldY = static_cast<float>(y * tileHeight);

                float opacity = layer->opacity;
                float r = 1.0f, g = 1.0f, b = 1.0f, a = opacity;

                bool flipH = GIDHelper::IsHorizontallyFlipped(gid);
                bool flipV = GIDHelper::IsVerticallyFlipped(gid);
                bool flipD = GIDHelper::IsDiagonallyFlipped(gid);

                TileVertex v0, v1, v2, v3;

                if (flipD) {
                    v0 = TileVertex(worldX, worldY, flipH ? u1 : u0, flipV ? v1 : v0, textureIndex, r, g, b, a);
                    v1 = TileVertex(worldX, worldY + tileHeight, flipH ? u1 : u0, flipV ? v0 : v1, textureIndex, r, g, b, a);
                    v2 = TileVertex(worldX + tileWidth, worldY + tileHeight, flipH ? u0 : u1, flipV ? v0 : v1, textureIndex, r, g, b, a);
                    v3 = TileVertex(worldX + tileWidth, worldY, flipH ? u0 : u1, flipV ? v1 : v0, textureIndex, r, g, b, a);
                } else {
                    v0 = TileVertex(worldX, worldY, flipH ? u1 : u0, flipV ? v1 : v0, textureIndex, r, g, b, a);
                    v1 = TileVertex(worldX, worldY + tileHeight, flipH ? u1 : u0, flipV ? v0 : v1, textureIndex, r, g, b, a);
                    v2 = TileVertex(worldX + tileWidth, worldY + tileHeight, flipH ? u0 : u1, flipV ? v0 : v1, textureIndex, r, g, b, a);
                    v3 = TileVertex(worldX + tileWidth, worldY, flipH ? u0 : u1, flipV ? v1 : v0, textureIndex, r, g, b, a);
                }

                chunk.vertices.insert(chunk.vertices.end(), {
                    v0.x, v0.y, v0.u, v0.v, v0.texIndex, v0.r, v0.g, v0.b, v0.a,
                    v1.x, v1.y, v1.u, v1.v, v1.texIndex, v1.r, v1.g, v1.b, v1.a,
                    v2.x, v2.y, v2.u, v2.v, v2.texIndex, v2.r, v2.g, v2.b, v2.a,
                    v3.x, v3.y, v3.u, v3.v, v3.texIndex, v3.r, v3.g, v3.b, v3.a
                });

                uint32_t i0 = vertexOffset + 0;
                uint32_t i1 = vertexOffset + 1;
                uint32_t i2 = vertexOffset + 2;
                uint32_t i3 = vertexOffset + 3;

                chunk.indices.insert(chunk.indices.end(), {i0, i1, i2, i0, i2, i3});

                vertexOffset += 4;
                chunk.hasData = true;
            }
        }
    }

    chunk.vertexCount = vertexOffset;
    chunk.indexCount = static_cast<uint32_t>(chunk.indices.size());
}

// ============================================================================
// 纹理管理
// ============================================================================

bool TilemapRenderer::LoadTilesetTextures() {
    if (!m_tilemap || !m_tilemap->IsLoaded() || !m_device) {
        return false;
    }

    const TileMap* map = m_tilemap->GetMap();
    if (!map) return false;

    m_tilesetTextures.clear();
    m_tilesetToTextureIndex.clear();

    // TODO: 实际从图块集路径加载纹理
    // 这里需要使用 ResourceManager 加载纹理

    for (const auto& tileset : map->tilesets) {
        if (!tileset) continue;

        // 暂时创建占位符
        int index = static_cast<int>(m_tilesetTextures.size());
        if (index < 16) {
            m_tilesetToTextureIndex[tileset.get()] = index;
            m_tilesetTextures.push_back(nullptr); // 占位符
        }
    }

    return true;
}

int TilemapRenderer::GetTilesetTextureIndex(const Tileset* tileset) const {
    auto it = m_tilesetToTextureIndex.find(tileset);
    if (it != m_tilesetToTextureIndex.end()) {
        return it->second;
    }
    return -1;
}

// ============================================================================
// 动画管理
// ============================================================================

void TilemapRenderer::RegisterAnimatedTiles() {
    m_animatedTiles.clear();

    if (!m_tilemap || !m_tilemap->IsLoaded()) {
        return;
    }

    const TileMap* map = m_tilemap->GetMap();
    if (!map) return;

    // 遍历所有图块集，查找有动画的瓦片
    for (const auto& tileset : map->tilesets) {
        if (!tileset) continue;

        for (const auto& [id, tile] : tileset->tiles) {
            if (tile.HasAnimation()) {
                // 查找使用此瓦片的位置
                // TODO: 遍历所有层，找到使用此瓦片的位置
                for (auto* layer : m_tilemap->GetTileLayers()) {
                    if (!layer) continue;

                    for (int y = 0; y < layer->tileData.height; ++y) {
                        for (int x = 0; x < layer->tileData.width; ++x) {
                            uint32_t gid = layer->tileData.GetGid(x, y);
                            uint32_t pureGid = GIDHelper::GetPureGid(gid);

                            if (pureGid == static_cast<uint32_t>(tileset->firstGid + id)) {
                                AnimatedTileInfo info;
                                info.x = x;
                                info.y = y;
                                info.tile = &tile;
                                info.frameTimer = 0.0f;
                                info.currentFrame = 0;
                                info.baseGid = tileset->firstGid;
                                m_animatedTiles.push_back(info);
                            }
                        }
                    }
                }
            }
        }
    }
}

// ============================================================================
// 辅助方法
// ============================================================================

void TilemapRenderer::GetTileUV(
    const Tileset* tileset,
    int localId,
    float& u0, float& v0, float& u1, float& v1
) const {
    if (!tileset) {
        u0 = v0 = u1 = v1 = 0.0f;
        return;
    }

    tileset->GetTileUV(localId, u0, v0, u1, v1);
}

void TilemapRenderer::GetTilePosition(int x, int y, float& worldX, float& worldY) const {
    if (!m_tilemap || !m_tilemap->IsLoaded()) {
        worldX = worldY = 0.0f;
        return;
    }

    int tileWidth = m_tilemap->GetTileWidth();
    int tileHeight = m_tilemap->GetTileHeight();

    switch (m_tilemap->GetOrientation()) {
        case Orientation::Orthogonal:
            worldX = static_cast<float>(x * tileWidth);
            worldY = static_cast<float>(y * tileHeight);
            break;

        case Orientation::Isometric:
            worldX = static_cast<float>((x - y) * tileWidth / 2);
            worldY = static_cast<float>((x + y) * tileHeight / 2);
            break;

        case Orientation::Staggered:
        case Orientation::Hexagonal:
            // TODO: 实现交错和六边形坐标转换
            worldX = static_cast<float>(x * tileWidth);
            worldY = static_cast<float>(y * tileHeight);
            break;

        default:
            worldX = static_cast<float>(x * tileWidth);
            worldY = static_cast<float>(y * tileHeight);
            break;
    }
}

void TilemapRenderer::CreateMaterial() {
    if (!m_device) {
        return;
    }

    // TODO: 创建瓦片地图材质
    // 需要加载着色器并设置管线状态

    m_materialDirty = false;
}

} // namespace PrismaEngine
