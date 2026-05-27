#pragma once

#include "Export.h"
#include "math/MathTypes.h"
#include "interfaces/RenderTypes.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IRenderDevice.h"
#include <memory>
#include <vector>

namespace Prisma {
    struct Node;
}

namespace Prisma {
namespace Graphic {

class ITexture;
class RenderCommandContext;
class OrthographicCamera;

/* 2D 绘图系统 (Graphics2D) */
class ENGINE_API Graphics2D {
public:
    struct Statistics {
        uint32_t DrawCalls = 0;
        uint32_t QuadCount = 0;

        uint32_t GetTotalVertexCount() const { return QuadCount * 4; }
        uint32_t GetTotalIndexCount() const { return QuadCount * 6; }
    };

    static void Initialize();
    static void Shutdown();

    // ========== 绘制生命周期 ==========

    static void Begin(const OrthographicCamera& camera);
    static void End();
    
    // UI 模式：使用屏幕空间坐标
    static void BeginUI();
    static void EndUI();

    // ========== 绘制 API (带排序顺序) ==========

    // 绘制实心矩形
    static void DrawQuad(const Vector2& position, const Vector2& size, const Prisma::Color& color, int sortingOrder = 0);
    static void DrawQuad(const Matrix4& transform, const Prisma::Color& color, int sortingOrder = 0);

    // 绘制纹理矩形 (Sprite)
    static void DrawSprite(const Vector2& position, const Vector2& size, const std::shared_ptr<ITexture>& texture, int sortingOrder = 0, const Prisma::Color& tint = {1.0f, 1.0f, 1.0f, 1.0f});
    static void DrawSprite(const Matrix4& transform, const std::shared_ptr<ITexture>& texture, int sortingOrder = 0, const Prisma::Color& tint = {1.0f, 1.0f, 1.0f, 1.0f});
    
    // 绘制纹理矩形 (带 UV)
    static void DrawSprite(const Vector2& position, const Vector2& size, const std::shared_ptr<ITexture>& texture, const Vector2 uv[4], int sortingOrder = 0, const Prisma::Color& tint = {1.0f, 1.0f, 1.0f, 1.0f});
    static void DrawSprite(const Matrix4& transform, const std::shared_ptr<ITexture>& texture, const Vector2 uv[4], int sortingOrder = 0, const Prisma::Color& tint = {1.0f, 1.0f, 1.0f, 1.0f});

    // 绘制文本
    static void DrawString(const std::string& text, const Vector2& position, float scale = 1.0f, const Prisma::Color& color = {1.0f, 1.0f, 1.0f, 1.0f}, int sortingOrder = 0);
    static float GetStringWidth(const std::string& text, float scale = 1.0f);

    // ========== 内部渲染接口 (供 Pipeline 调用) ==========

    static void Execute(ICommandBuffer* cmd, IRenderDevice* device);

    // ========== 统计与配置 ==========

    static Statistics GetStats();
    static void SetLightTexture(const std::shared_ptr<ITexture>& texture);

private:
    static void StartBatch();
    static void NextBatch();

    struct Renderer2DData;
    static Renderer2DData* s_Data;
};

} // namespace Graphic
} // namespace Prisma
