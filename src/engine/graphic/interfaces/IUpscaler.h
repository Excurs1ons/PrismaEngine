#pragma once

#include "RenderTypes.h"
#include "math/MathTypes.h"
#include <string>
#include <vector>

namespace PrismaEngine::Graphic {

// 前置声明
class IDeviceContext;
class ITexture;
class IRenderTarget;
class IDepthStencil;

/// @brief 超分辨率质量模式 / Upscaler Quality Mode
enum class UpscalerQuality : uint32_t {
    None = 0,              // 不使用超分 / No upscaling
    UltraQuality = 1,      // 1.3x (最高质量 / Highest quality)
    Quality = 2,           // 1.5x
    Balanced = 3,          // 1.7x
    Performance = 4,       // 2.0x
    UltraPerformance = 5   // 3.0x (最高性能 / Highest performance)
};

/// @brief 超分辨率技术类型 / Upscaler Technology Type
enum class UpscalerTechnology : uint32_t {
    None = 0,
    FSR = 1,    // AMD FidelityFX Super Resolution
    DLSS = 2,   // NVIDIA Deep Learning Super Sampling
    TSR = 3     // Temporal Super Resolution
};

/// @brief 超分辨率输入描述 / Upscaler Input Description
struct UpscalerInputDesc {
    ITexture* colorTexture = nullptr;       // 当前帧颜色 (HDR recommended) / Current frame color
    ITexture* depthTexture = nullptr;       // 深度缓冲 / Depth buffer
    ITexture* motionVectorTexture = nullptr; // 运动矢量 (RG16_FLOAT recommended) / Motion vectors
    ITexture* normalTexture = nullptr;      // 法线 (optional, for better quality) / Normals (optional)
    ITexture* exposureTexture = nullptr;    // 曝光 (optional, DLSS 需要) / Exposure (optional, DLSS requires)

    float jitterX = 0.0f;                   // 亚像素抖动 X / Sub-pixel jitter X
    float jitterY = 0.0f;                   // 亚像素抖动 Y / Sub-pixel jitter Y
    float deltaTime = 0.0f;                 // 帧时间 / Frame time
    bool resetAccumulation = false;         // 是否重置历史累积 / Reset history accumulation

    // 相机信息（用于运动矢量计算验证）/ Camera info (for motion vector validation)
    struct {
        PrismaMath::mat4 view;
        PrismaMath::mat4 projection;
        PrismaMath::mat4 viewProjection;
        PrismaMath::mat4 prevViewProjection;
    } camera;
};

/// @brief 超分辨率输出描述 / Upscaler Output Description
struct UpscalerOutputDesc {
    IRenderTarget* outputTarget = nullptr;  // 输出渲染目标 / Output render target
    uint32_t outputWidth = 0;               // 输出宽度 / Output width
    uint32_t outputHeight = 0;              // 输出高度 / Output height
    bool sharpnessEnabled = false;          // 是否启用锐化 / Enable sharpening
    float sharpness = 0.5f;                 // 锐化强度 [0-1] / Sharpness intensity
};

/// @brief 超分辨率初始化描述 / Upscaler Initialization Description
struct UpscalerInitDesc {
    uint32_t renderWidth = 0;               // 渲染分辨率宽度 / Render resolution width
    uint32_t renderHeight = 0;              // 渲染分辨率高度 / Render resolution height
    uint32_t displayWidth = 0;              // 显示分辨率宽度 / Display resolution width
    uint32_t displayHeight = 0;             // 显示分辨率高度 / Display resolution height
    UpscalerQuality quality = UpscalerQuality::Quality;
    bool enableHDR = false;                 // 是否启用 HDR / Enable HDR
    uint32_t maxFramesInFlight = 2;         // 最大帧数（用于资源池）/ Max frames in flight (for resource pool)
};

/// @brief 超分辨率器信息 / Upscaler Information
struct UpscalerInfo {
    UpscalerTechnology technology;
    std::string name;                       // 技术名称 / Technology name
    std::string version;                    // 版本号 / Version
    std::vector<UpscalerQuality> supportedQualities; // 支持的质量模式 / Supported quality modes
    bool requiresMotionVectors;             // 是否需要运动矢量 / Requires motion vectors
    bool requiresDepth;                     // 是否需要深度 / Requires depth
    bool requiresExposure;                  // 是否需要曝光 / Requires exposure
    bool requiresNormal;                    // 是否需要法线 / Requires normals
    uint32_t minRenderWidth;                // 最小渲染分辨率 / Minimum render resolution
    uint32_t minRenderHeight;               // 最小渲染分辨率 / Minimum render resolution
};

/// @brief 超分辨率器统一接口 / Upscaler Unified Interface
/// 提供所有超分辨率技术必须实现的统一接口
class IUpscaler {
public:
    virtual ~IUpscaler() = default;

    // === 生命周期管理 / Lifecycle Management ===

    /// @brief 初始化超分辨率器 / Initialize upscaler
    /// @param desc 初始化描述 / Initialization description
    /// @return 是否初始化成功 / Whether initialization succeeded
    virtual bool Initialize(const UpscalerInitDesc& desc) = 0;

    /// @brief 关闭超分辨率器 / Shutdown upscaler
    virtual void Shutdown() = 0;

    /// @brief 检查是否已初始化 / Check if initialized
    virtual bool IsInitialized() const = 0;

    // === 渲染执行 / Rendering ===

    /// @brief 执行超分辨率 / Execute upscaling
    /// @param context 设备上下文 / Device context
    /// @param input 输入描述 / Input description
    /// @param output 输出描述 / Output description
    /// @return 是否执行成功 / Whether execution succeeded
    virtual bool Upscale(IDeviceContext* context,
                         const UpscalerInputDesc& input,
                         const UpscalerOutputDesc& output) = 0;

    // === 配置管理 / Configuration Management ===

    /// @brief 设置质量模式 / Set quality mode
    /// @param quality 质量模式 / Quality mode
    /// @return 是否设置成功 / Whether setting succeeded
    virtual bool SetQualityMode(UpscalerQuality quality) = 0;

    /// @brief 获取当前质量模式 / Get current quality mode
    virtual UpscalerQuality GetQualityMode() const = 0;

    /// @brief 设置渲染分辨率 / Set render resolution (需要重新初始化 / Requires re-initialization)
    /// @param width 渲染宽度 / Render width
    /// @param height 渲染高度 / Render height
    /// @return 是否设置成功 / Whether setting succeeded
    virtual bool SetRenderResolution(uint32_t width, uint32_t height) = 0;

    /// @brief 设置显示分辨率 / Set display resolution (需要重新初始化 / Requires re-initialization)
    /// @param width 显示宽度 / Display width
    /// @param height 显示高度 / Display height
    /// @return 是否设置成功 / Whether setting succeeded
    virtual bool SetDisplayResolution(uint32_t width, uint32_t height) = 0;

    /// @brief 获取推荐的渲染分辨率 / Get recommended render resolution
    /// @param quality 质量模式 / Quality mode
    /// @param displayWidth 显示宽度 / Display width
    /// @param displayHeight 显示高度 / Display height
    /// @param outWidth 输出渲染宽度 / Output render width
    /// @param outHeight 输出渲染高度 / Output render height
    virtual void GetRecommendedRenderResolution(UpscalerQuality quality,
                                                 uint32_t displayWidth,
                                                 uint32_t displayHeight,
                                                 uint32_t& outWidth,
                                                 uint32_t& outHeight) const = 0;

    // === 查询接口 / Query Interface ===

    /// @brief 获取超分辨率器信息 / Get upscaler information
    virtual UpscalerInfo GetInfo() const = 0;

    /// @brief 是否支持质量模式 / Check if quality mode is supported
    virtual bool IsQualityModeSupported(UpscalerQuality quality) const = 0;

    /// @brief 获取当前性能统计 / Get current performance statistics
    struct PerformanceStats {
        float avgUpscaleTime = 0.0f;        // 平均超分时间 (ms) / Average upscale time
        float avgFrameTime = 0.0f;          // 平均帧时间 (ms) / Average frame time
        uint32_t currentFPS = 0;            // 当前 FPS / Current FPS
    };
    virtual PerformanceStats GetPerformanceStats() const = 0;

    // === 资源管理 / Resource Management ===

    /// @brief 当窗口大小改变时调用 / Call when window size changes
    /// @param newWidth 新宽度 / New width
    /// @param newHeight 新高度 / New height
    /// @return 是否调整成功 / Whether resize succeeded
    virtual bool OnResize(uint32_t newWidth, uint32_t newHeight) = 0;

    /// @brief 释放并重新分配资源 / Release and reallocate resources
    virtual void ReleaseResources() = 0;

    // === 调试功能 / Debug Functions ===

    /// @brief 获取调试信息字符串 / Get debug information string
    virtual std::string GetDebugInfo() const = 0;

    /// @brief 是否需要重置历史（场景切换时调用）/ Reset history (call on scene change)
    virtual void ResetHistory() = 0;
};

} // namespace PrismaEngine::Graphic
