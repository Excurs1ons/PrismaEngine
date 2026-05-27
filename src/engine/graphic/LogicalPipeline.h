#pragma once

#include "../Export.h"
#include "interfaces/IPass.h"
#include "interfaces/IRenderTarget.h"
#include "interfaces/IGBuffer.h"
#include <vector>
#include <memory>
#include <algorithm>

namespace Prisma::Graphic {

class ENGINE_API LogicalPipeline : public ILogicalPipeline {
public:
    LogicalPipeline(const char* name);
    virtual ~LogicalPipeline();

    // === IPipeline 接口实现 ===

    const char* GetName() const override { return m_name.c_str(); }

    bool AddPass(IPass* pass) override;
    bool RemovePass(IPass* pass) override;

    size_t GetPassCount() const override { return m_passes.size(); }
    IPass* GetPass(size_t index) const override;
    IPass* FindPass(const char* name) const override;

    void Execute(const PassExecutionContext& context) override;

    void SetViewport(uint32_t width, uint32_t height) override;
    void SetRenderTarget(IRenderTarget* renderTarget) override;
    void SetDepthStencil(IDepthStencil* depthStencil) override;

    // === 扩展功能 ===

    // 清空所有 Pass
    void Clear();

    // 按优先级排序 Pass
    void SortByPriority();

    // 是否按优先级自动排序
    void SetAutoSort(bool autoSort) { m_autoSort = autoSort; }

    // 获取当前视口宽度
    uint32_t GetWidth() const { return m_width; }

    // 获取当前视口高度
    uint32_t GetHeight() const { return m_height; }

    // 获取主渲染目标
    IRenderTarget* GetRenderTarget() const { return m_renderTarget; }

    // 获取深度模板
    IDepthStencil* GetDepthStencil() const { return m_depthStencil; }

protected:
    std::string m_name;
    std::vector<IPass*> m_passes;
    bool m_autoSort;

    // 全局渲染目标
    IRenderTarget* m_renderTarget;
    IDepthStencil* m_depthStencil;

    // 视口尺寸
    uint32_t m_width;
    uint32_t m_height;
};

// 逻辑前向渲染管线
/// 常见的前向渲染管线实现（非图形 API Pipeline State Object）
class LogicalForwardPipeline : public LogicalPipeline {
public:
    LogicalForwardPipeline();
    ~LogicalForwardPipeline() override = default;

    // 设置主渲染目标
    /// 传递给所有子 Pass
    void SetRenderTarget(IRenderTarget* renderTarget) override;

    // 执行管线
    /// 按顺序执行所有 Pass
    void Execute(const PassExecutionContext& context) override;
};

// 逻辑延迟渲染管线
/// 延迟渲染管线实现（非图形 API Pipeline State Object）
class ENGINE_API LogicalDeferredPipeline : public LogicalPipeline {
public:
    LogicalDeferredPipeline();
    ~LogicalDeferredPipeline() override = default;

    // 设置 G-Buffer
    void SetGBuffer(IGBuffer* gBuffer) { m_gBuffer = gBuffer; }

    // 获取 G-Buffer
    IGBuffer* GetGBuffer() const { return m_gBuffer; }

    // 执行 Pipeline
    /// 1. Geometry Pass 2. Lighting Pass 3. Transparency Pass
    void Execute(const PassExecutionContext& context) override;

protected:
    IGBuffer* m_gBuffer;
};

} // namespace Prisma::Graphic
