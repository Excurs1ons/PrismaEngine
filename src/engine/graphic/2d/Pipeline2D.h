#pragma once

#include "interfaces/IPipeline.h"
#include <memory>
#include <vector>

namespace Prisma::Graphic {

class CanvasPass2D;
class Light2DPass;
class OpaquePass;
class UIPass2D;
class PostProcessPass2D;

/**
 * @brief 纯 2D 渲染管线 (Standard Pipeline 2D)
 * 
 * 专注于 2D 游戏的性能与特性，不包含 3D 渲染开销。
 * 由 RenderSystem 根据 RenderMode::Mode2D 自动创建。
 */
class Pipeline2D : public IPipeline {
public:
    Pipeline2D();
    ~Pipeline2D() override;

    int Initialize(IRenderDevice* device) override;
    void Shutdown() override;
    void Execute(const RenderContext& ctx) override;

private:
    IRenderDevice* m_device = nullptr;

    // 2D 专属 Pass 集合
    std::shared_ptr<Light2DPass> m_lightPass;        // 生成光照贴图
    std::shared_ptr<OpaquePass> m_opaquePass;        // 渲染 Renderer2D 内容
    std::shared_ptr<CanvasPass2D> m_canvasPass;      // 渲染世界空间的 Canvas (Graphics2D)
    std::shared_ptr<PostProcessPass2D> m_ppPass;     // 2D 后处理
    std::shared_ptr<UIPass2D> m_uiPass;              // 渲染屏幕空间的 UI
};

} // namespace Prisma::Graphic
