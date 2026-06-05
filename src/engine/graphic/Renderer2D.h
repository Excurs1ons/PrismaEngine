#pragma once

#include "Export.h"
#include "math/MathTypes.h"
#include "interfaces/RenderTypes.h"
#include "ICamera.h"
#include <memory>
#include <vector>

namespace Prisma {
    struct Node;
}

namespace Prisma {
namespace Graphic {

class ITexture;
class SpriteAnimationComponent;
class RenderCommandContext;

/* 2D 渲染器 (静态接口) */
class ENGINE_API Renderer2D {
public:
    struct Statistics {
        uint32_t DrawCalls = 0;
        uint32_t QuadCount = 0;

        uint32_t GetTotalVertexCount() const { return QuadCount * 4; }
        uint32_t GetTotalIndexCount() const { return QuadCount * 6; }
    };

    static void Initialize();
    static void Shutdown();

    // ========== 渲染生命周期 ==========

    static void BeginScene(const ICamera& camera);
    static void EndScene();
    static void Flush();

    // UI 覆盖层
    static void BeginUI();
    static void EndUI();

    // Gizmo 覆盖层（不受光照影响，使用管线内部 gizmo 相机）
    static void BeginGizmo();
    static void EndGizmo();

    // ========== 绘制接口 ==========

    // 绘制 SoA 节点 (从 EntityManager 自动提取数据)
    static void DrawNodesSoA();
    static void DrawNode(Node node, const Prisma::Color& tintColor = {1.0f, 1.0f, 1.0f, 1.0f});

    // 调试与 Gizmos
    static void DrawLine(const Vector2& start, const Vector2& end, const Prisma::Color& color, float thickness = 1.0f);
    static void DrawRect(const Vector2& position, const Vector2& size, const Prisma::Color& color, float thickness = 1.0f);

    // 绘制实心矩形
    static void DrawQuad(const Vector2& position, const Vector2& size, const Prisma::Color& color);
    static void DrawQuad(const Matrix4& transform, const Prisma::Color& color);

    // 绘制纹理矩形 (Sprite)
    static void DrawQuad(const Vector2& position, const Vector2& size, const std::shared_ptr<ITexture>& texture, const Prisma::Color& tintColor = {1.0f, 1.0f, 1.0f, 1.0f});
    static void DrawQuad(const Matrix4& transform, const std::shared_ptr<ITexture>& texture, const Prisma::Color& tintColor = {1.0f, 1.0f, 1.0f, 1.0f});

    // 绘制纹理矩形 (带 UV)
    static void DrawQuad(const Vector2& position, const Vector2& size, const std::shared_ptr<ITexture>& texture, const Vector2 uv[4], const Prisma::Color& tintColor = {1.0f, 1.0f, 1.0f, 1.0f});
    static void DrawQuad(const Matrix4& transform, const std::shared_ptr<ITexture>& texture, const Vector2 uv[4], const Prisma::Color& tintColor = {1.0f, 1.0f, 1.0f, 1.0f});

    // 绘制动画精灵 (使用 SpriteAnimationComponent 的当前帧 UV)
    static void DrawAnimatedSprite(const Vector2& position, const Vector2& size, 
        const std::shared_ptr<ITexture>& texture, 
        const SpriteAnimationComponent& anim,
        const Prisma::Color& tintColor = {1.0f, 1.0f, 1.0f, 1.0f});

    // 绘制文本 (使用内置像素字体或提供的图集)
    static void DrawString(const std::string& text, const Vector2& position, float scale = 1.0f, const Prisma::Color& color = {1.0f, 1.0f, 1.0f, 1.0f});
    static float GetStringWidth(const std::string& text, float scale = 1.0f);

    // ========== 统计数据 ==========

    static void ResetStats();
    static Statistics GetStats();

    // 控制合批开关
    static void SetBatchingEnabled(bool enabled);
    static bool IsBatchingEnabled();

    // 设置最大合批四边形数（需在 Initialize 前调用）
    static void SetMaxBatchQuads(uint32_t maxBatchQuads);

    // 设置光照纹理
    static void SetLightTexture(const std::shared_ptr<ITexture>& texture);

    // 全局 1x1 白纹理（无纹理绘制时的默认 fallback）
    static std::shared_ptr<ITexture> GetWhiteTexture();

private:
    static void StartBatch();
    static void NextBatch();

    struct Renderer2DData;
    static Renderer2DData* s_Data;
};

} // namespace Graphic
} // namespace Prisma
