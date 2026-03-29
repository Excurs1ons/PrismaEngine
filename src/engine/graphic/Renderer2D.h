#pragma once

#include "math/MathTypes.h"
#include "interfaces/RenderTypes.h"
#include <memory>
#include <vector>

namespace Prisma {
namespace Graphic {

class ITexture;
class RenderCommandContext;
class OrthographicCamera;

/**
 * @brief 2D 渲染器 (静态接口)
 * 提供高性能的 2D 形状和精灵渲染功能，支持批处理
 */
class Renderer2D {
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

    static void BeginScene(const OrthographicCamera& camera);
    static void EndScene();
    static void Flush();

    // ========== 绘制接口 ==========

    // 绘制实心矩形
    static void DrawQuad(const Vector2& position, const Vector2& size, const Prisma::Color& color);
    static void DrawQuad(const Matrix4& transform, const Prisma::Color& color);

    // 绘制纹理矩形 (Sprite)
    static void DrawQuad(const Vector2& position, const Vector2& size, const std::shared_ptr<ITexture>& texture, const Prisma::Color& tintColor = {1.0f, 1.0f, 1.0f, 1.0f});
    static void DrawQuad(const Matrix4& transform, const std::shared_ptr<ITexture>& texture, const Prisma::Color& tintColor = {1.0f, 1.0f, 1.0f, 1.0f});

    // 绘制纹理矩形 (带 UV)
    static void DrawQuad(const Vector2& position, const Vector2& size, const std::shared_ptr<ITexture>& texture, const Vector2 uv[4], const Prisma::Color& tintColor = {1.0f, 1.0f, 1.0f, 1.0f});
    static void DrawQuad(const Matrix4& transform, const std::shared_ptr<ITexture>& texture, const Vector2 uv[4], const Prisma::Color& tintColor = {1.0f, 1.0f, 1.0f, 1.0f});

    // ========== 统计数据 ==========

    static void ResetStats();
    static Statistics GetStats();

private:
    static void StartBatch();
    static void NextBatch();
};

} // namespace Graphic
} // namespace Prisma
