#pragma once

#include "2d/Graphics2D.h"
#include "core/Node.h"

namespace Prisma {
namespace Graphic {

/**
 * @brief 兼容层：将 Renderer2D 映射到 Graphics2D
 */
class ENGINE_API Renderer2D {
public:
    using Statistics = Graphics2D::Statistics;

    static void Initialize() { Graphics2D::Initialize(); }
    static void Shutdown() { Graphics2D::Shutdown(); }

    static void BeginScene(const OrthographicCamera& camera) { Graphics2D::Begin(camera); }
    static void EndScene() { Graphics2D::End(); }
    static void Flush() { /* Graphics2D 内部自动管理 Flush */ }

    static void BeginGizmo(const OrthographicCamera& camera) { Graphics2D::Begin(camera); }
    static void EndGizmo() { Graphics2D::End(); }

    static void BeginUI() { Graphics2D::BeginUI(); }
    static void EndUI() { Graphics2D::EndUI(); }

    static void DrawNodesSoA(); // 保持原 SoA 实现
    static void DrawNode(Node node, int layer = 0, const Prisma::Color& tint = {1,1,1,1});

    static void DrawLine(const Vector2& s, const Vector2& e, const Prisma::Color& c, float t = 1.0f, int l = 0);
    static void DrawRect(const Vector2& p, const Vector2& s, const Prisma::Color& c, float t = 1.0f, int l = 0);

    static void DrawQuad(const Vector2& p, const Vector2& s, const Prisma::Color& c, int l = 0) { Graphics2D::DrawQuad(p, s, c, l); }
    static void DrawQuad(const Matrix4& t, const Prisma::Color& c, int l = 0) { Graphics2D::DrawQuad(t, c, l); }

    static void DrawQuad(const Vector2& p, const Vector2& s, const std::shared_ptr<ITexture>& tex, const Prisma::Color& c) 
    { Graphics2D::DrawSprite(p, s, tex, 0, c); }

    static void DrawQuad(const Vector2& p, const Vector2& s, const std::shared_ptr<ITexture>& tex, int l = 0, const Prisma::Color& c = {1,1,1,1}) 
    { Graphics2D::DrawSprite(p, s, tex, l, c); }
    
    static void DrawQuad(const Matrix4& t, const std::shared_ptr<ITexture>& tex, int l = 0, const Prisma::Color& c = {1,1,1,1}) 
    { Graphics2D::DrawSprite(t, tex, l, c); }

    static void DrawQuad(const Vector2& p, const Vector2& s, const std::shared_ptr<ITexture>& tex, const Vector2 uv[4], const Prisma::Color& c = {1,1,1,1}) 
    { Graphics2D::DrawSprite(p, s, tex, uv, 0, c); }

    static void DrawQuad(const Matrix4& t, const std::shared_ptr<ITexture>& tex, const Vector2 uv[4], const Prisma::Color& c = {1,1,1,1}) 
    { Graphics2D::DrawSprite(t, tex, uv, 0, c); }

    static void DrawString(const std::string& t, const Vector2& p, float s = 1.0f, const Prisma::Color& c = {1,1,1,1}) 
    { Graphics2D::DrawString(t, p, s, c, 0); }

    static float GetStringWidth(const std::string& t, float s = 1.0f) { return Graphics2D::GetStringWidth(t, s); }

    static Statistics GetStats() { return Graphics2D::GetStats(); }
    static void SetBatchingEnabled(bool) {}
    static bool IsBatchingEnabled() { return true; }
    static void SetLightTexture(const std::shared_ptr<ITexture>& t) { Graphics2D::SetLightTexture(t); }
};

} // namespace Graphic
} // namespace Prisma
