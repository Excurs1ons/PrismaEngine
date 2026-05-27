#pragma once

#include "graphic/LogicalPass.h"
#include "graphic/interfaces/IPass.h"
#include "graphic/interfaces/IDeviceContext.h"
#include "graphic/interfaces/IRenderTarget.h"
#include "math/MathTypes.h"
#include "ForwardRenderPassBase.h"
#include <memory>
#include <vector>

namespace Prisma::Graphic {

// 透明物体逻辑 Pass
/// 使用深度缓冲和 Alpha 混合渲染透明物体
class TransparentPass : public ForwardRenderPass {
public:
    // 渲染统计
    struct RenderStats {
        uint32_t drawCalls = 0;
        uint32_t triangles = 0;
        uint32_t transparentObjects = 0;
    };

public:
    TransparentPass();
    ~TransparentPass() override = default;

    // === IPass 接口实现 ===

    // 执行 Pass
    void Execute(const PassExecutionContext& context) override;

    // 更新 Pass 数据
    void Update(Prisma::Timestep ts) override;

    // === 渲染设置 ===

    // 设置深度写入
    void SetDepthWrite(bool enable) { m_depthWrite = enable; }

    // 获取深度写入状态
    bool GetDepthWrite() const { return m_depthWrite; }

    // 设置深度测试
    void SetDepthTest(bool enable) { m_depthTest = enable; }

    // 获取深度测试状态
    bool GetDepthTest() const { return m_depthTest; }

    // === 渲染统计 ===

    // 获取渲染统计
    const RenderStats& GetRenderStats() const { return m_stats; }
    RenderStats& GetRenderStats() { return m_stats; }

    // 重置渲染统计
    void ResetStats() { m_stats = RenderStats(); }

private:
    // 渲染状态
    bool m_depthWrite;
    bool m_depthTest;

    // 渲染统计
    RenderStats m_stats;
};

} // namespace Prisma::Graphic
