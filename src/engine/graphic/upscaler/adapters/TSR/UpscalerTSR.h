#pragma once

#include "interfaces/IUpscaler.h"
#include <memory>

namespace PrismaEngine::Graphic {

// 前置声明
class IRenderDevice;
class ITexture;
class IBuffer;

/// @brief 时序超分辨率 (TSR) 适配器
/// 基于 UE5 TSR 算法原理的自实现方案
/// 无需外部 SDK，完全自主实现
class UpscalerTSR : public IUpscaler {
public:
    UpscalerTSR();
    ~UpscalerTSR() override;

    // === IUpscaler 接口实现 ===

    // 生命周期管理
    bool Initialize(const UpscalerInitDesc& desc) override;
    void Shutdown() override;
    bool IsInitialized() const override { return m_initialized; }

    // 渲染执行
    bool Upscale(IDeviceContext* context,
                 const UpscalerInputDesc& input,
                 const UpscalerOutputDesc& output) override;

    // 配置管理
    bool SetQualityMode(UpscalerQuality quality) override;
    UpscalerQuality GetQualityMode() const override { return m_quality; }

    bool SetRenderResolution(uint32_t width, uint32_t height) override;
    bool SetDisplayResolution(uint32_t width, uint32_t height) override;

    void GetRecommendedRenderResolution(UpscalerQuality quality,
                                         uint32_t displayWidth,
                                         uint32_t displayHeight,
                                         uint32_t& outWidth,
                                         uint32_t& outHeight) const override;

    // 查询接口
    UpscalerInfo GetInfo() const override;
    bool IsQualityModeSupported(UpscalerQuality quality) const override;
    PerformanceStats GetPerformanceStats() const override { return m_stats; }

    // 资源管理
    bool OnResize(uint32_t newWidth, uint32_t newHeight) override;
    void ReleaseResources() override;

    // 调试功能
    std::string GetDebugInfo() const override;
    void ResetHistory() override;

    // === TSR 特定参数 ===

    /// @brief 设置时序稳定性因子
    /// @param stability 稳定性因子 [0-1]，越高越稳定但可能有重影
    void SetTemporalStability(float stability) { m_temporalStability = stability; }

    /// @brief 获取时序稳定性因子
    float GetTemporalStability() const { return m_temporalStability; }

private:
    bool CreateResources();
    void ReleaseResources();
    bool CreateShaders();
    void ReleaseShaders();

    // 渲染设备
    IRenderDevice* m_device = nullptr;

    // GPU 资源
    IBuffer* m_constantBuffer = nullptr;
    ITexture* m_historyColor = nullptr;      // 历史颜色
    ITexture* m_historyDepth = nullptr;      // 历史深度

    // 配置
    UpscalerQuality m_quality = UpscalerQuality::Quality;
    uint32_t m_renderWidth = 0;
    uint32_t m_renderHeight = 0;
    uint32_t m_displayWidth = 0;
    uint32_t m_displayHeight = 0;
    bool m_enableHDR = false;
    uint32_t m_maxFramesInFlight = 2;

    // TSR 特定参数
    float m_temporalStability = 0.95f;  // 时序稳定性因子
    float m_sharpness = 0.5f;           // 锐化强度

    // 状态
    bool m_initialized = false;
    bool m_needReset = true;

    // 性能统计
    PerformanceStats m_stats;

    // 帧计数
    uint32_t m_frameIndex = 0;
};

} // namespace PrismaEngine::Graphic
