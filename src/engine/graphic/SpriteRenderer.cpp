#include "SpriteRenderer.h"
#include "RenderCommandContext.h"
#include "Renderer2D.h"
#include "ComponentRegistry.h"
#include <glaze/glaze.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

// ── Glaze 元数据 ──
template <>
struct glz::meta<Prisma::Graphic::SpriteRenderer::Data> {
    static constexpr auto value = glz::object(
        "color", &Prisma::Graphic::SpriteRenderer::Data::color,
        "position", &Prisma::Graphic::SpriteRenderer::Data::position,
        "size", &Prisma::Graphic::SpriteRenderer::Data::size,
        "rotation", &Prisma::Graphic::SpriteRenderer::Data::rotation
    );
};

// ── 注册 ──
namespace {
    bool registered = []() {
        auto& reg = Prisma::ComponentRegistry::Get();
        reg.Register<Prisma::Graphic::SpriteRenderer>("SpriteRenderer");
        reg.RegisterSerializable("SpriteRenderer",
            [](const Prisma::Component& comp) -> std::string {
                const auto& typed = static_cast<const Prisma::Graphic::SpriteRenderer&>(comp);
                auto data = typed.GetData();
                std::string json;
                glz::write_json(data, json);
                return json;
            },
            [](Prisma::Component& comp, const std::string& json) {
                auto& typed = static_cast<Prisma::Graphic::SpriteRenderer&>(comp);
                Prisma::Graphic::SpriteRenderer::Data data;
                glz::read_json(data, json);
                typed.SetData(data);
            }
        );
        return true;
    }();
}

namespace Prisma {
namespace Graphic {

SpriteRenderer::SpriteRenderer() {}

Prisma::Graphic::SpriteRenderer::Data SpriteRenderer::GetData() const {
    return {{
                m_color.r, m_color.g, m_color.b, m_color.a
            },
            {
                m_position.x, m_position.y
            },
            {
                m_size.x, m_size.y
            },
            m_rotation};
}

void SpriteRenderer::SetData(const Data& d) {
    m_color = {d.color[0], d.color[1], d.color[2], d.color[3]};
    m_position = {d.position[0], d.position[1]};
    m_size = {d.size[0], d.size[1]};
    m_rotation = d.rotation;
}

void SpriteRenderer::Render(RenderCommandContext* context) {
    if (!m_visible || !m_texture) {
        return;
    }

    // 【修复】改用 Renderer2D::DrawQuad 替代手动顶点创建
    // 这确保了 SpriteRenderer 使用统一的 2D 渲染管道（批处理 + OpaquePass 着色器）

    // 计算纹理坐标（支持子矩形和翻转）
    Vector2 uv[4];
    if (m_useSpriteRect) {
        uv[0] = {m_spriteRect.x, m_spriteRect.y};
        uv[1] = {m_spriteRect.x + m_spriteRect.z, m_spriteRect.y};
        uv[2] = {m_spriteRect.x + m_spriteRect.z, m_spriteRect.y + m_spriteRect.w};
        uv[3] = {m_spriteRect.x, m_spriteRect.y + m_spriteRect.w};
    } else {
        uv[0] = {0.0f, 0.0f};
        uv[1] = {1.0f, 0.0f};
        uv[2] = {1.0f, 1.0f};
        uv[3] = {0.0f, 1.0f};
    }

    // 应用翻转
    if (m_flipX) {
        std::swap(uv[0].x, uv[1].x);
        std::swap(uv[2].x, uv[3].x);
    }
    if (m_flipY) {
        std::swap(uv[0].y, uv[3].y);
        std::swap(uv[1].y, uv[2].y);
    }

    // 构建变换矩阵：Translate(Rotate(Scale))，以精灵中心为原点
    // Renderer2D 的 Quad Mesh 中心在 (-0.5,-0.5)～(0.5,0.5)
    Vector2 center = m_position + m_size * 0.5f;
    Matrix4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(center, 0.0f));

    if (std::abs(m_rotation) > 0.001f) {
        transform = glm::rotate(transform, glm::radians(m_rotation), glm::vec3(0.0f, 0.0f, 1.0f));
    }

    transform = glm::scale(transform, glm::vec3(m_size, 1.0f));

    // 通过 Renderer2D 的统一管道提交
    Renderer2D::DrawQuad(transform, m_texture, uv, m_color);
}

} // namespace Graphic
} // namespace Prisma
