#include "TilemapRenderer.h"
#include "graphic/Renderer2D.h"
#include "Logger.h"
#include <cmath>

namespace Prisma::Tilemap {

void TilemapRenderer::Render(const Graphic::OrthographicCamera& camera) {
    if (!m_tilemap || !m_texture) return;

    uint32_t tileSize = m_tilemap->GetTileSize();
    float worldTileSize = static_cast<float>(tileSize);

    // 计算相机可见范围
    glm::vec3 camPos = camera.GetPosition();
    float camHalfW = camera.GetAspectRatio() * camera.GetZoom() * 0.5f;
    float camHalfH = camera.GetZoom() * 0.5f;
    float camLeft   = camPos.x - camHalfW;
    float camRight  = camPos.x + camHalfW;
    float camBottom = camPos.y - camHalfH;
    float camTop    = camPos.y + camHalfH;

    // 扩展一个 tile 避免边缘闪烁
    camLeft   -= worldTileSize;
    camRight  += worldTileSize;
    camBottom -= worldTileSize;
    camTop    += worldTileSize;

    int startX = std::max(0, static_cast<int>(std::floor(camLeft / worldTileSize)));
    int startY = std::max(0, static_cast<int>(std::floor(camBottom / worldTileSize)));
    int endX = std::min(static_cast<int>(m_tilemap->GetWidth()),
                        static_cast<int>(std::ceil(camRight / worldTileSize)));
    int endY = std::min(static_cast<int>(m_tilemap->GetHeight()),
                        static_cast<int>(std::ceil(camTop / worldTileSize)));

    uint32_t cols = m_tilemap->GetTileSet().GetColumns();
    if (cols == 0) cols = 1;

    // 渲染每个可见图层
    for (uint32_t l = 0; l < m_tilemap->GetLayerCount(); l++) {
        const auto* layer = m_tilemap->GetLayer(l);
        if (!layer || !layer->visible) continue;

        for (int y = startY; y < endY; y++) {
            for (int x = startX; x < endX; x++) {
                uint32_t tileId = layer->GetTile(static_cast<uint32_t>(x),
                                                  static_cast<uint32_t>(y));
                if (tileId == 0) continue;

                // 世界坐标 (tile 中心)
                float wx = static_cast<float>(x) * worldTileSize + worldTileSize * 0.5f;
                float wy = static_cast<float>(y) * worldTileSize + worldTileSize * 0.5f;

                // 图集中的 UV
                const auto* def = m_tilemap->GetTileSet().GetTileDef(tileId);
                uint32_t texIndex = def ? def->texIndex : 0;
                uint32_t tx = texIndex % cols;
                uint32_t ty = texIndex / cols;

                float u0 = static_cast<float>(tx) / static_cast<float>(cols);
                float v0 = static_cast<float>(ty) / static_cast<float>(cols);
                float u1 = u0 + 1.0f / static_cast<float>(cols);
                float v1 = v0 + 1.0f / static_cast<float>(cols);

                Vector2 uv[4] = {
                    {u0, v0}, // 左上
                    {u1, v0}, // 右上
                    {u1, v1}, // 右下
                    {u0, v1}  // 左下
                };

                Graphic::Renderer2D::DrawQuad(
                    Vector2(wx, wy),
                    Vector2(worldTileSize, worldTileSize),
                    m_texture,
                    uv,
                    Color(1.0f)
                );
            }
        }
    }
}

} // namespace Prisma::Tilemap
