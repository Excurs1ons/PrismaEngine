#pragma once

#include "graphic/LogicalPass.h"
#include "graphic/interfaces/IPass.h"
#include "graphic/interfaces/IDeviceContext.h"
#include "graphic/interfaces/IRenderTarget.h"
#include "math/MathTypes.h"
#include <memory>

namespace PrismaEngine::Graphic {

/// @brief 深度预渲染逻辑 Pass
/// 用于提前构建深度缓冲，优化后续渲染的遮挡剔除
/// 在前向渲染管线中最早执行
class DepthPrePass : public ForwardRenderPass {
public:
    // 渲染统计
    struct RenderStats {
        uint32_t drawCalls = 0;
        uint32_t triangles = 0;
        uint32_t objects = 0;
    };

public:
    DepthPrePass();
    ~DepthPrePass() override = default;

    // === IPass 接口实现 ===

    /// @brief 执行 Pass
    /// @param context 执行上下文
    void Execute(const PassExecutionContext& context) override;

    /// @brief 更新 Pass 数据
    /// @param deltaTime 时间增量
    void Update(float deltaTime) override;

    // === 渲染统计 ===

    /// @brief 获取渲染统计
    const RenderStats& GetRenderStats() const { return m_stats; }
    RenderStats& GetRenderStats() { return m_stats; }

    /// @brief 重置渲染统计
    void ResetStats() { m_stats = RenderStats(); }

private:
    // 渲染统计
    RenderStats m_stats;
};

} // namespace PrismaEngine::Graphic
