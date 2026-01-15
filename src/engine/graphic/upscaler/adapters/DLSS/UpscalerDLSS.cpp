#include "UpscalerDLSS.h"
#include "DLSSResources.h"
#include "interfaces/IRenderDevice.h"
#include "interfaces/ITexture.h"
#include <cstring>
#include <cmath>

// Streamline SDK includes
// 注意：这些头文件路径将在 FetchContent 下载 SDK 后可用
#if defined(PRISMA_ENABLE_UPSCALER_DLSS)
    // #include <sl.h>
    // #include <sl_dlss.h>
    // #include <sl_interop.h>
#endif

namespace PrismaEngine::Graphic {

// ========== DLSS 质量模式对应的缩放因子 ==========
// DLSS 4/4.5 质量模式映射表
static constexpr float DLSS_SCALE_FACTORS[] = {
    1.0f,  // Off
    1.3f,  // Ultra Quality
    1.5f,  // Quality
    1.7f,  // Balanced
    2.0f,  // Performance
    3.0f   // Ultra Performance
};

static constexpr uint32_t DLSS_MIN_RENDER_WIDTH = 320;
static constexpr uint32_t DLSS_MIN_RENDER_HEIGHT = 180;


// ========== UpscalerDLSS Implementation ==========

UpscalerDLSS::UpscalerDLSS()
    : m_resources(std::make_unique<DLSSResources>())
{
}

UpscalerDLSS::~UpscalerDLSS() {
    Shutdown();
}

bool UpscalerDLSS::Initialize(const UpscalerInitDesc& desc) {
    if (m_initialized) {
        return true;
    }

    // 验证参数
    if (desc.renderWidth == 0 || desc.renderHeight == 0 ||
        desc.displayWidth == 0 || desc.displayHeight == 0) {
        return false;
    }

    if (desc.renderWidth < DLSS_MIN_RENDER_WIDTH ||
        desc.renderHeight < DLSS_MIN_RENDER_HEIGHT) {
        return false;
    }

    // 保存配置
    m_renderWidth = desc.renderWidth;
    m_renderHeight = desc.renderHeight;
    m_displayWidth = desc.displayWidth;
    m_displayHeight = desc.displayHeight;
    m_quality = desc.quality;
    m_enableHDR = desc.enableHDR;
    m_maxFramesInFlight = desc.maxFramesInFlight;
    m_frameIndex = 0;

    // 创建 DLSS 上下文
    if (!CreateDLSSContext()) {
        Shutdown();
        return false;
    }

    // 创建着色器
    if (!CreateShaders()) {
        Shutdown();
        return false;
    }

    // 初始化资源管理器
    if (!m_resources->Initialize(m_device, m_renderWidth, m_renderHeight,
                                  m_displayWidth, m_displayHeight,
                                  m_maxFramesInFlight)) {
        Shutdown();
        return false;
    }

    m_initialized = true;
    m_needReset = false;

    return true;
}

void UpscalerDLSS::Shutdown() {
    if (!m_initialized) {
        return;
    }

    ReleaseShaders();
    DestroyDLSSContext();

    if (m_resources) {
        m_resources->Release();
    }

    m_streamlineContext = nullptr;
    m_dlssFeature = nullptr;
    m_device = nullptr;
    m_initialized = false;
}

bool UpscalerDLSS::Upscale(IDeviceContext* context,
                            const UpscalerInputDesc& input,
                            const UpscalerOutputDesc& output) {
    if (!m_initialized || !context) {
        return false;
    }

    // 验证必需输入
    if (!input.colorTexture || !input.depthTexture || !input.motionVectorTexture) {
        return false;
    }

    // DLSS 需要曝光纹理（根据 GetInfo() 中的 requiresExposure=true）
    if (!input.exposureTexture) {
        // TODO: 可以在这里生成默认曝光
        return false;
    }

    // TODO: 调用 Streamline DLSS API 进行超分辨率
    //
    // SLDLSSParams dlssParams = {};
    // dlssParams.pColorTexture = GetSLResource(input.colorTexture);
    // dlssParams.pDepthTexture = GetSLResource(input.depthTexture);
    // dlssParams.pMotionVectorsTexture = GetSLResource(input.motionVectorTexture);
    // dlssParams.pExposureTexture = GetSLResource(input.exposureTexture);
    // dlssParams.pOutputTexture = GetSLResource(output.outputTarget);
    // dlssParams.jitterOffset.x = input.jitterX;
    // dlssParams.jitterOffset.y = input.jitterY;
    // dlssParams.resetAccumulation = input.resetAccumulation || m_needReset;
    // dlssParams.sharpness = output.sharpnessEnabled ? output.sharpness : 0.0f;
    //
    // SLDLSSResult result = slDLSSExecute(m_dlssFeature, &dlssParams);
    // if (result != SL_DLSS_RESULT_OK) {
    //     return false;
    // }

    m_needReset = false;
    m_frameIndex++;

    return true;
}

bool UpscalerDLSS::SetQualityMode(UpscalerQuality quality) {
    if (!IsQualityModeSupported(quality)) {
        return false;
    }

    if (m_quality != quality) {
        m_quality = quality;
        m_needReset = true;
    }

    return true;
}

bool UpscalerDLSS::SetRenderResolution(uint32_t width, uint32_t height) {
    if (width < DLSS_MIN_RENDER_WIDTH || height < DLSS_MIN_RENDER_HEIGHT) {
        return false;
    }

    if (m_renderWidth != width || m_renderHeight != height) {
        m_renderWidth = width;
        m_renderHeight = height;
        m_needReset = true;

        // 重新创建资源
        if (m_resources && m_device) {
            m_resources->Release();
            if (!m_resources->Initialize(m_device, m_renderWidth, m_renderHeight,
                                          m_displayWidth, m_displayHeight,
                                          m_maxFramesInFlight)) {
                return false;
            }
        }
    }

    return true;
}

bool UpscalerDLSS::SetDisplayResolution(uint32_t width, uint32_t height) {
    if (m_displayWidth != width || m_displayHeight != height) {
        m_displayWidth = width;
        m_displayHeight = height;
        m_needReset = true;

        // 重新创建资源
        if (m_resources && m_device) {
            m_resources->Release();
            if (!m_resources->Initialize(m_device, m_renderWidth, m_renderHeight,
                                          m_displayWidth, m_displayHeight,
                                          m_maxFramesInFlight)) {
                return false;
            }
        }
    }

    return true;
}

void UpscalerDLSS::GetRecommendedRenderResolution(UpscalerQuality quality,
                                                    uint32_t displayWidth,
                                                    uint32_t displayHeight,
                                                    uint32_t& outWidth,
                                                    uint32_t& outHeight) const {
    UpscalerHelper::CalculateRenderResolution(quality, displayWidth, displayHeight,
                                               outWidth, outHeight);
}

UpscalerInfo UpscalerDLSS::GetInfo() const {
    UpscalerInfo info;
    info.technology = UpscalerTechnology::DLSS;
    info.name = "NVIDIA DLSS";
    info.version = "4.5";
    info.supportedQualities = {
        UpscalerQuality::UltraQuality,
        UpscalerQuality::Quality,
        UpscalerQuality::Balanced,
        UpscalerQuality::Performance,
        UpscalerQuality::UltraPerformance
    };
    info.requiresMotionVectors = true;
    info.requiresDepth = true;
    info.requiresExposure = true;  // DLSS 需要曝光
    info.requiresNormal = false;
    info.minRenderWidth = DLSS_MIN_RENDER_WIDTH;
    info.minRenderHeight = DLSS_MIN_RENDER_HEIGHT;
    return info;
}

bool UpscalerDLSS::IsQualityModeSupported(UpscalerQuality quality) const {
    switch (quality) {
        case UpscalerQuality::UltraQuality:
        case UpscalerQuality::Quality:
        case UpscalerQuality::Balanced:
        case UpscalerQuality::Performance:
        case UpscalerQuality::UltraPerformance:
            return true;
        default:
            return false;
    }
}

bool UpscalerDLSS::OnResize(uint32_t newWidth, uint32_t newHeight) {
    return SetDisplayResolution(newWidth, newHeight);
}

void UpscalerDLSS::ReleaseResources() {
    if (m_resources) {
        m_resources->Release();
    }
    m_needReset = true;
}

std::string UpscalerDLSS::GetDebugInfo() const {
    std::string info = "DLSS 4.5 Upscaler:\n";
    info += "  Initialized: " + std::string(m_initialized ? "Yes" : "No") + "\n";
    info += "  Render Resolution: " + std::to_string(m_renderWidth) + "x" +
            std::to_string(m_renderHeight) + "\n";
    info += "  Display Resolution: " + std::to_string(m_displayWidth) + "x" +
            std::to_string(m_displayHeight) + "\n";
    info += "  Quality Mode: " + UpscalerHelper::GetQualityName(m_quality) + "\n";
    info += "  Frame Index: " + std::to_string(m_frameIndex) + "\n";
    return info;
}

void UpscalerDLSS::ResetHistory() {
    m_needReset = true;
    m_frameIndex = 0;
}

bool UpscalerDLSS::CreateDLSSContext() {
#if defined(PRISMA_ENABLE_UPSCALER_DLSS)
    // TODO: 初始化 Streamline DLSS 上下文
    //
    // SLInputStreamCallback callbacks = {};
    // SLSetupDesc setupDesc = {};
    // setupDesc.api = (PRISMA_ENABLE_RENDER_DX12) ? SL_API_DX12 : SL_API_VULKAN;
    // setupDesc.callbacks = &callbacks;
    //
    // SLResult result = slSetFeatureLevel(SL_FEATURE_LEVEL_EXPERIMENTAL);
    // if (result != SL_RESULT_OK) return false;
    //
    // result = slInit(&setupDesc, &m_streamlineContext);
    // if (result != SL_RESULT_OK) return false;
    //
    // 创建 DLSS feature
    // SLDLSSFeatureDesc dlssDesc = {};
    // dlssDesc.renderWidth = m_renderWidth;
    // dlssDesc.renderHeight = m_renderHeight;
    // dlssDesc.displayWidth = m_displayWidth;
    // dlssDesc.displayHeight = m_displayHeight;
    // dlssDesc.quality = GetDLSSQualityMode(m_quality);
    //
    // result = slDLSSCreate(m_streamlineContext, &dlssDesc, &m_dlssFeature);
    // return result == SL_RESULT_OK;

    // 占位符：暂时返回 true
    return true;
#else
    return false;
#endif
}

void UpscalerDLSS::DestroyDLSSContext() {
#if defined(PRISMA_ENABLE_UPSCALER_DLSS)
    // TODO: 销毁 Streamline DLSS 上下文
    // if (m_dlssFeature) {
    //     slDLSSDestroy(m_dlssFeature);
    //     m_dlssFeature = nullptr;
    // }
    // if (m_streamlineContext) {
    //     slShutdown(m_streamlineContext);
    //     m_streamlineContext = nullptr;
    // }
#endif
}

bool UpscalerDLSS::CreateShaders() {
    // DLSS 通过 Streamline SDK 管理着色器
    // 不需要手动创建
    return true;
}

void UpscalerDLSS::ReleaseShaders() {
    // Streamline SDK 管理着色器生命周期
}

UpscalerDLSS::DLSSQualityMode UpscalerDLSS::GetDLSSQualityMode(UpscalerQuality quality) const {
    switch (quality) {
        case UpscalerQuality::None:
            return DLSSQualityMode::Off;
        case UpscalerQuality::UltraQuality:
            return DLSSQualityMode::UltraQuality;
        case UpscalerQuality::Quality:
            return DLSSQualityMode::Quality;
        case UpscalerQuality::Balanced:
            return DLSSQualityMode::Balanced;
        case UpscalerQuality::Performance:
            return DLSSQualityMode::Performance;
        case UpscalerQuality::UltraPerformance:
            return DLSSQualityMode::UltraPerformance;
        default:
            return DLSSQualityMode::Quality;
    }
}

} // namespace PrismaEngine::Graphic
