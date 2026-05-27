#pragma once

#include "Export.h"
#include "Component.h"
#include "math/MathTypes.h"
#include <array>
#include <memory>

namespace Prisma {
namespace Graphic {

class ITexture;
class RenderCommandContext;

// 2D 精灵渲染组件
class ENGINE_API SpriteRenderer : public Component {
public:
    SpriteRenderer();
    virtual ~SpriteRenderer() = default;

    // ========== 纹理与区域 ==========

    void SetTexture(std::shared_ptr<ITexture> texture) { m_texture = texture; }
    std::shared_ptr<ITexture> GetTexture() const { return m_texture; }

    void SetSpriteRect(float x, float y, float width, float height) {
        m_spriteRect = {x, y, width, height};
        m_useSpriteRect = true;
    }

    void UseFullTexture() { m_useSpriteRect = false; }

    // ========== 颜色与翻转 ==========

    void SetColor(const Prisma::Color& color) { m_color = color; }
    const Prisma::Color& GetColor() const { return m_color; }

    void SetFlipX(bool flip) { m_flipX = flip; }
    void SetFlipY(bool flip) { m_flipY = flip; }

    // ========== 变换 ==========

    void SetPosition(const Vector2& position) { m_position = position; }
    const Vector2& GetPosition() const { return m_position; }

    void SetSize(const Vector2& size) { m_size = size; }
    void SetSize(float width, float height) { m_size = {width, height}; }
    const Vector2& GetSize() const { return m_size; }

    void SetRotation(float rotation) { m_rotation = rotation; }
    float GetRotation() const { return m_rotation; }

    void SetOpacity(float opacity) { m_color.a = opacity; }
    float GetOpacity() const { return m_color.a; }

    void SetVisible(bool visible) { m_visible = visible; }
    bool IsVisible() const { return m_visible; }

    // ========== 序列化 ==========

    struct Data {
        std::array<float, 4> color = {1,1,1,1};
        std::array<float, 2> position = {0,0};
        std::array<float, 2> size = {100,100};
        float rotation = 0.0f;
    };

    ComponentId GetComponentId() const override { return GetComponentTypeId<SpriteRenderer>(); }
    const char* GetComponentTypeName() const override { return "SpriteRenderer"; }
    Data GetData() const;
    void SetData(const Data& d);

    // ========== 渲染 ==========

    virtual void Render(RenderCommandContext* context);

    // ========== Component 接口实现 ==========

    virtual void Initialize() override {}
    virtual void Update(Timestep /*ts*/) override {}
    virtual void Shutdown() override {}

private:
    std::shared_ptr<ITexture> m_texture;
    Vector4 m_spriteRect = {0.0f, 0.0f, 1.0f, 1.0f};
    bool m_useSpriteRect = false;

    Prisma::Color m_color = {1.0f, 1.0f, 1.0f, 1.0f};
    
    Vector2 m_position = {0.0f, 0.0f};
    Vector2 m_size = {1.0f, 1.0f};
    float m_rotation = 0.0f;

    bool m_flipX = false;
    bool m_flipY = false;
    bool m_visible = true;
};

} // namespace Graphic
} // namespace Prisma
