#include "SpriteRenderer.h"
#include "RenderCommandContext.h"
#include <glm/glm.hpp>
#include <algorithm>

namespace Prisma {
namespace Graphic {

struct SpriteVertex {
    Vector2 position;
    Vector2 texCoord;
    Prisma::Color color;
};

SpriteRenderer::SpriteRenderer() {
}

void SpriteRenderer::Render(RenderCommandContext* context) {
    if (!m_visible || !m_texture || !context) {
        return;
    }

    // 纹理坐标
    Vector2 texCoords[4];
    if (m_useSpriteRect) {
        texCoords[0] = {m_spriteRect.x, m_spriteRect.y};
        texCoords[1] = {m_spriteRect.x + m_spriteRect.z, m_spriteRect.y};
        texCoords[2] = {m_spriteRect.x + m_spriteRect.z, m_spriteRect.y + m_spriteRect.w};
        texCoords[3] = {m_spriteRect.x, m_spriteRect.y + m_spriteRect.w};
    } else {
        texCoords[0] = {0.0f, 0.0f};
        texCoords[1] = {1.0f, 0.0f};
        texCoords[2] = {1.0f, 1.0f};
        texCoords[3] = {0.0f, 1.0f};
    }

    // 应用翻转
    if (m_flipX) {
        std::swap(texCoords[0].x, texCoords[1].x);
        std::swap(texCoords[2].x, texCoords[3].x);
    }
    if (m_flipY) {
        std::swap(texCoords[0].y, texCoords[3].y);
        std::swap(texCoords[1].y, texCoords[2].y);
    }

    // 顶点位置
    Vector2 p1 = m_position;
    Vector2 p2 = m_position + Vector2(m_size.x, 0.0f);
    Vector2 p3 = m_position + m_size;
    Vector2 p4 = m_position + Vector2(0.0f, m_size.y);

    // 如果有旋转
    if (std::abs(m_rotation) > 0.001f) {
        float rad = glm::radians(m_rotation);
        float cosA = std::cos(rad);
        float sinA = std::sin(rad);
        Vector2 center = m_position + m_size * 0.5f;

        auto rotate = [&](Vector2 p) -> Vector2 {
            Vector2 local = p - center;
            return Vector2(
                local.x * cosA - local.y * sinA + center.x,
                local.x * sinA + local.y * cosA + center.y
            );
        };

        p1 = rotate(p1);
        p2 = rotate(p2);
        p3 = rotate(p3);
        p4 = rotate(p4);
    }

    // 设置顶点数据
    SpriteVertex vertices[4];
    vertices[0] = {p1, texCoords[0], m_color};
    vertices[1] = {p2, texCoords[1], m_color};
    vertices[2] = {p3, texCoords[2], m_color};
    vertices[3] = {p4, texCoords[3], m_color};

    // 索引数据
    uint32_t indices[] = { 0, 1, 2, 0, 2, 3 };

    // 提交到上下文
    context->SetTexture(m_texture.get(), 0);
    context->SetVertexData(vertices, sizeof(vertices), sizeof(SpriteVertex));
    context->SetIndexData(indices, sizeof(indices), true);
    context->DrawIndexed(6);
}

} // namespace Graphic
} // namespace Prisma
