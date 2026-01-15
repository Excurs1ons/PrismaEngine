#pragma once

#include "graphic/interfaces/IPass.h"
#include "graphic/interfaces/IUpscaler.h"
#include "graphic/interfaces/IGBuffer.h"
#include "math/MathTypes.h"
#include <memory>

namespace PrismaEngine::Graphic {

// 前置声明
class IUpscaler;
class ITexture;

/// @brief 超分辨率 Pass
/// 在 CompositionPass 之后执行，对最终图像进行超分辨率处理
class UpscalerPass : public IPass {
public:
    UpscalerPass();
    ~UpscalerPass() override;

    // === IPass 接口实现 ===

    const char* GetName() const override { return "UpscalerPass"; }

    bool IsEnabled() const override { return m_enabled; }
    void SetEnabled(bool enabled) override { m_enabled = enabled; }

    void SetRenderTarget(IRenderTarget* renderTarget) override;
    void SetDepthStencil(IDepthStencil* depthStencil) override;
    void SetViewport(uint32_t width, uint32_t height) override;

    void Update(float deltaTime) override;
    void Execute(const PassExecutionContext& context) override;

    uint32_t GetPriority() const override { return m_priority; }
    void SetPriority(uint32_t priority) override { m_priority = priority; }

    // === 超分辨率配置 ===

    /// @brief 设置超分辨率技术
    /// @param technology 技术类型
    /// @return 是否设置成功
    bool SetUpscaler(UpscalerTechnology technology);

    /// @brief 获取当前超分辨率技术
    UpscalerTechnology GetCurrentTechnology() const;

    /// @brief 设置质量模式
    /// @param quality 质量模式
    void SetQualityMode(UpscalerQuality quality);

    /// @brief 获取当前质量模式
    UpscalerQuality GetQualityMode() const { return m_quality; }

    // === 输入资源设置 ===

    /// @brief 设置颜色输入（来自 CompositionPass）
    void SetColorInput(ITextureRenderTarget* colorTarget) { m_colorInput = colorTarget; }

    /// @brief 设置深度输入
    void SetDepthInput(ITextureRenderTarget* depth) { m_depthInput = depth; }

    /// @brief 设置运动矢量输入
    void SetMotionVectorInput(ITextureRenderTarget* motionVectors) { m_motionVectors = motionVectors; }

    /// @brief 设置法线输入（optional）
    void SetNormalInput(ITextureRenderTarget* normal) { m_normalInput = normal; }

    // === 输出配置 ===

    /// @brief 设置输出渲染目标（通常是 SwapChain）
    void SetOutputTarget(IRenderTarget* output) { m_outputTarget = output; }

    // === 相机信息 ===

    /// @brief 更新相机信息（用于运动矢量验证）
    void UpdateCameraInfo(const PrismaMath::mat4& view,
                          const PrismaMath::mat4& projection,
                          const PrismaMath::mat4& prevViewProjection);

    // === 抖动序列 ===

    /// @brief 获取当前帧的抖动偏移
    void GetJitterOffset(float& x, float& y) const;

    // === 调试功能 ===

    /// @brief 设置调试可视化模式
    enum class DebugMode {
        None,
        ShowMotionVectors,
        ShowDepth,
        ShowInputResolution,
        ShowOutputResolution
    };
    void SetDebugMode(DebugMode mode) { m_debugMode = mode; }

    /// @brief 获取超分辨率器（用于直接访问）
    IUpscaler* GetUpscaler() const { return m_upscaler; }

private:
    bool InitializeUpscaler();
    void ReleaseUpscaler();

    // 超分辨率器
    IUpscaler* m_upscaler = nullptr;
    UpscalerTechnology m_currentTechnology = UpscalerTechnology::None;
    UpscalerQuality m_quality = UpscalerQuality::Quality;

    // 输入资源
    ITextureRenderTarget* m_colorInput = nullptr;
    ITextureRenderTarget* m_depthInput = nullptr;
    ITextureRenderTarget* m_motionVectors = nullptr;
    ITextureRenderTarget* m_normalInput = nullptr;

    // 输出资源
    IRenderTarget* m_outputTarget = nullptr;

    // 相机信息
    PrismaMath::mat4 m_viewMatrix;
    PrismaMath::mat4 m_projMatrix;
    PrismaMath::mat4 m_viewProjMatrix;
    PrismaMath::mat4 m_prevViewProjMatrix;

    // 抖动序列
    int m_jitterIndex = 0;
    float m_jitterX = 0.0f;
    float m_jitterY = 0.0f;

    // 调试
    DebugMode m_debugMode = DebugMode::None;

    // 内部资源
    uint32_t m_renderWidth = 0;
    uint32_t m_renderHeight = 0;
    uint32_t m_displayWidth = 0;
    uint32_t m_displayHeight = 0;

    // Pass 状态
    bool m_enabled = true;
    uint32_t m_priority = 1000;  // 高优先级，在最后执行
};

} // namespace PrismaEngine::Graphic
