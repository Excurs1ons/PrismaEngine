#pragma once

#include "interfaces/IPass.h"
#include "interfaces/IRenderTarget.h"
#include "math/MathTypes.h"
#include <string>
#include <memory>

namespace PrismaEngine::Graphic {

/// @brief 逻辑 Pass 基类
/// 提供通用的 Pass 实现，所有逻辑 Pass 应继承此类
/// 不包含任何具体图形 API 类型
class LogicalPass : public IPass {
public:
    LogicalPass(const char* name);
    virtual ~LogicalPass() = default;

    // === IPass 接口实现 ===

    const char* GetName() const override { return m_name.c_str(); }
    bool IsEnabled() const override { return m_enabled; }
    void SetEnabled(bool enabled) override { m_enabled = enabled; }

    void SetRenderTarget(IRenderTarget* renderTarget) override { m_renderTarget = renderTarget; }
    void SetDepthStencil(IDepthStencil* depthStencil) override { m_depthStencil = depthStencil; }
    void SetViewport(uint32_t width, uint32_t height) override;

    void Update(float deltaTime) override { /* 默认空实现 */ }

    uint32_t GetPriority() const override { return m_priority; }
    void SetPriority(uint32_t priority) override { m_priority = priority; }

    // === 辅助方法 ===

    /// @brief 获取渲染目标
    IRenderTarget* GetRenderTarget() const { return m_renderTarget; }

    /// @brief 获取深度模板
    IDepthStencil* GetDepthStencil() const { return m_depthStencil; }

    /// @brief 获取视口宽度
    uint32_t GetWidth() const { return m_width; }

    /// @brief 获取视口高度
    uint32_t GetHeight() const { return m_height; }

    /// @brief 获取宽高比
    float GetAspectRatio() const { return m_height > 0 ? static_cast<float>(m_width) / m_height : 1.0f; }

    /// @brief 获取最后一次更新的 delta time
    float GetDeltaTime() const { return m_deltaTime; }

    /// @brief 获取累计时间
    float GetTotalTime() const { return m_totalTime; }

protected:
    /// @brief 设置清除颜色
    void SetClearColor(float r, float g, float b, float a) {
        m_clearColor[0] = r;
        m_clearColor[1] = g;
        m_clearColor[2] = b;
        m_clearColor[3] = a;
    }

    /// @brief 获取清除颜色
    const float* GetClearColor() const { return m_clearColor; }

    /// @brief 更新时间
    void UpdateTime(float deltaTime) {
        m_deltaTime = deltaTime;
        m_totalTime += deltaTime;
    }

    // Pass 成员变量
    std::string m_name;
    uint32_t m_priority;
    bool m_enabled;

    // 渲染目标
    IRenderTarget* m_renderTarget;
    IDepthStencil* m_depthStencil;

    // 视口
    uint32_t m_width;
    uint32_t m_height;

    // 时间
    float m_deltaTime;
    float m_totalTime;

    // 清除颜色
    float m_clearColor[4];
};

} // namespace PrismaEngine::Graphic
