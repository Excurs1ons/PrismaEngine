#pragma once

#include "interfaces/IUpscaler.h"
#include <memory>

namespace PrismaEngine::Graphic {

// 前置声明
class IRenderDevice;
class ITexture;
class IBuffer;
class FSRResources;

/// @brief AMD FidelityFX Super Resolution 3.1 适配器
/// FSR 3.1 超分辨率技术实现
class UpscalerFSR : public IUpscaler {
public:
    UpscalerFSR();
    ~UpscalerFSR() override;

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

private:
    bool CreateFSRContext();
    void DestroyFSRContext();
    bool CreateShaders();
    void ReleaseShaders();

    // FSR 上下文 (FFX SDK context)
    void* m_fsrContext = nullptr;  // FfxFsr3Context*

    // 渲染设备
    IRenderDevice* m_device = nullptr;

    // FSR 资源管理器
    std::unique_ptr<FSRResources> m_resources;

    // 配置
    UpscalerQuality m_quality = UpscalerQuality::Quality;
    uint32_t m_renderWidth = 0;
    uint32_t m_renderHeight = 0;
    uint32_t m_displayWidth = 0;
    uint32_t m_displayHeight = 0;
    bool m_enableHDR = false;
    uint32_t m_maxFramesInFlight = 2;

    // 状态
    bool m_initialized = false;
    bool m_needReset = true;

    // 性能统计
    PerformanceStats m_stats;

    // 帧计数
    uint32_t m_frameIndex = 0;

    // 抖动偏移 (Halton 序列)
    float m_jitterX = 0.0f;
    float m_jitterY = 0.0f;
    int m_jitterIndex = 0;

    // FSR 特定的质量模式映射
    enum class FSRQualityMode {
        NativeAA = 0,     // 无超分
        Quality = 1,      // 1.5x
        Balanced = 2,     // 1.7x
        Performance = 3,  // 2.0x
        UltraPerformance = 4  // 3.0x
    };

    // 将通用质量模式转换为 FSR 质量模式
    FSRQualityMode GetFSRQualityMode(UpscalerQuality quality) const;
};

} // namespace PrismaEngine::Graphic
