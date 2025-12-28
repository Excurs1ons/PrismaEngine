#pragma once

/// @file ScriptableRenderPipeline.h
/// @deprecated 此文件已被弃用，请使用 graphic/LogicalPipeline.h 中的 LogicalPipeline 类
/// @note 旧版 ScriptableRenderPipeline 类将被 LogicalPipeline 替代
/// @note 此文件仅为向后兼容保留，将在未来版本中移除

#include "RenderPass.h"
#include <vector>
#include <memory>

// 前向声明
namespace PrismaEngine::Graphic {
    class RenderBackend;
    class RenderCommandContext;
}

/// @deprecated 使用 graphic/LogicalPipeline.h 中的 LogicalPipeline 替代
class [[deprecated("Use LogicalPipeline from graphic/LogicalPipeline.h instead")]] ScriptableRenderPipeline
{
public:
    ScriptableRenderPipeline();
    ~ScriptableRenderPipeline();

    /// @deprecated 使用 LogicalPipeline::Initialize() 或子类的 Initialize() 方法
    virtual bool Initialize(PrismaEngine::Graphic::RenderBackend* renderBackend);

    /// @deprecated 使用 LogicalPipeline::Execute() 替代
    virtual void Shutdown();

    /// @deprecated 使用 LogicalPipeline::Execute() 替代
    virtual void Execute();

    /// @deprecated 使用 LogicalPipeline::AddPass() 替代
    virtual void AddRenderPass(std::shared_ptr<RenderPass> renderPass);

    /// @deprecated 使用 LogicalPipeline::RemovePass() 替代
    virtual void RemoveRenderPass(std::shared_ptr<RenderPass> renderPass);

    /// @deprecated 通过 IPass 接口获取设备
    virtual PrismaEngine::Graphic::RenderBackend* GetRenderBackend() const { return m_renderBackend; }

    /// @deprecated 使用 IPass::SetViewport() 替代
    virtual void SetViewportSize(uint32_t width, uint32_t height);

private:
    PrismaEngine::Graphic::RenderBackend* m_renderBackend = nullptr;
    std::vector<std::shared_ptr<RenderPass>> m_renderPasses;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    PrismaEngine::Graphic::RenderCommandContext* m_cachedContext = nullptr;
};
