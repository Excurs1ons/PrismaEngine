#pragma once

#include "graphic/LogicalPass.h"
#include "graphic/interfaces/IPass.h"
#include "graphic/interfaces/IDeviceContext.h"
#include "graphic/interfaces/IRenderTarget.h"
#include "graphic/interfaces/IGBuffer.h"
#include "math/MathTypes.h"
#include <memory>
#include <pipelines/forward/ForwardRenderPassBase.h>

namespace PrismaEngine::Graphic {

/// @brief 几何逻辑 Pass
/// 将场景几何信息渲染到 G-Buffer
class GeometryPass : public ForwardRenderPass {
public:
    // 渲染统计
    struct RenderStats {
        uint32_t drawCalls = 0;
        uint32_t triangles = 0;
        uint32_t objects = 0;
        uint32_t culledObjects = 0;
    };

public:
    GeometryPass();
    ~GeometryPass() override = default;

    // === IPass 接口实现 ===

    /// @brief 执行 Pass
    /// @param context 执行上下文
    void Execute(const PassExecutionContext& context) override;

    /// @brief 更新 Pass 数据
    /// @param deltaTime 时间增量
    void Update(float deltaTime) override;

    // === G-Buffer 设置 ===

    /// @brief 设置 G-Buffer
    /// @param gBuffer G-Buffer 接口指针
    void SetGBuffer(IGBuffer* gBuffer) { m_gBuffer = gBuffer; }

    /// @brief 获取 G-Buffer
    IGBuffer* GetGBuffer() const { return m_gBuffer; }

    // === 深度预渲染设置 ===

    /// @brief 设置是否启用深度预渲染
    /// @param enable 是否启用
    void SetDepthPrePass(bool enable) { m_depthPrePass = enable; }

    /// @brief 获取深度预渲染状态
    bool GetDepthPrePass() const { return m_depthPrePass; }

    // === 渲染统计 ===

    /// @brief 获取渲染统计
    const RenderStats& GetRenderStats() const { return m_stats; }
    RenderStats& GetRenderStats() { return m_stats; }

    /// @brief 重置渲染统计
    void ResetStats() { m_stats = RenderStats(); }

private:
    // G-Buffer
    IGBuffer* m_gBuffer;

    // 深度预渲染标志
    bool m_depthPrePass;

    // 渲染统计
    RenderStats m_stats;
};

} // namespace PrismaEngine::Graphic
