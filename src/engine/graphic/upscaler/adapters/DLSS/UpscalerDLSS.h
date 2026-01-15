#pragma once

#include "interfaces/IUpscaler.h"
#include <memory>

namespace PrismaEngine::Graphic {

// 前置声明
class IRenderDevice;
class ITexture;
class IBuffer;
class DLSSResources;

/// @brief NVIDIA DLSS 4/4.5 适配器
/// 通过 Streamline SDK 集成 DLSS 超分辨率技术
class UpscalerDLSS : public IUpscaler {
public:
    UpscalerDLSS();
    ~UpscalerDLSS() override;

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
    bool CreateDLSSContext();
    void DestroyDLSSContext();
    bool CreateShaders();
    void ReleaseShaders();

    // DLSS/Streamline 上下文
    void* m_streamlineContext = nullptr;  // SLContext*
    void* m_dlssFeature = nullptr;        // SLDLSSFeature*

    // 渲染设备
    IRenderDevice* m_device = nullptr;

    // DLSS 资源管理器
    std::unique_ptr<DLSSResources> m_resources;

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

    // DLSS 特定的质量模式映射
    enum class DLSSQualityMode {
        Off = 0,
        UltraQuality = 1,  // 1.3x
        Quality = 2,       // 1.5x
        Balanced = 3,      // 1.7x
        Performance = 4,   // 2.0x
        UltraPerformance = 5  // 3.0x
    };

    // 将通用质量模式转换为 DLSS 质量模式
    DLSSQualityMode GetDLSSQualityMode(UpscalerQuality quality) const;
};

} // namespace PrismaEngine::Graphic
