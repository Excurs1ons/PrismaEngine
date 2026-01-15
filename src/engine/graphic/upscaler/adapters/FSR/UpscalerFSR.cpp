#include "UpscalerFSR.h"
#include "FSRResources.h"
#include "interfaces/IRenderDevice.h"
#include "interfaces/ITexture.h"
#include <cstring>
#include <cmath>

// FidelityFX SDK includes
// 注意：这些头文件路径将在 FetchContent 下载 SDK 后可用
#if defined(PRISMA_ENABLE_UPSCALER_FSR)
    // #include <ffx_fsr3.h>
    // #include <ffx_fsr3_interface.h>
#endif

namespace PrismaEngine::Graphic {

// ========== FSR 3.1 质量模式对应的缩放因子 ==========
// FSR 3.1 质量模式映射表
static constexpr float FSR_SCALE_FACTORS[] = {
    1.0f,  // NativeAA (不使用超分)
    1.5f,  // Quality
    1.7f,  // Balanced
    2.0f,  // Performance
    3.0f   // Ultra Performance
};

static constexpr uint32_t FSR_MIN_RENDER_WIDTH = 320;
static constexpr uint32_t FSR_MIN_RENDER_HEIGHT = 180;

// ========== UpscalerFSR Implementation ==========

UpscalerFSR::UpscalerFSR()
    : m_resources(std::make_unique<FSRResources>())
{
}

UpscalerFSR::~UpscalerFSR() {
    Shutdown();
}

bool UpscalerFSR::Initialize(const UpscalerInitDesc& desc) {
    if (m_initialized) {
        return true;
    }

    // 验证参数
    if (desc.renderWidth == 0 || desc.renderHeight == 0 ||
        desc.displayWidth == 0 || desc.displayHeight == 0) {
        return false;
    }

    if (desc.renderWidth < FSR_MIN_RENDER_WIDTH ||
        desc.renderHeight < FSR_MIN_RENDER_HEIGHT) {
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

    // 创建 FSR 上下文
    if (!CreateFSRContext()) {
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

void UpscalerFSR::Shutdown() {
    if (!m_initialized) {
        return;
    }

    ReleaseShaders();
    DestroyFSRContext();

    if (m_resources) {
        m_resources->Release();
    }

    m_fsrContext = nullptr;
    m_device = nullptr;
    m_initialized = false;
}

bool UpscalerFSR::Upscale(IDeviceContext* context,
                           const UpscalerInputDesc& input,
                           const UpscalerOutputDesc& output) {
    if (!m_initialized || !context) {
        return false;
    }

    // 验证输入
    if (!input.colorTexture || !input.depthTexture || !output.outputTarget) {
        return false;
    }

    // 更新抖动序列 (Halton 2,3)
    UpscalerHelper::GenerateHaltonSequence(m_jitterIndex, m_jitterX, m_jitterY);
    m_jitterIndex = (m_jitterIndex + 1) % 16;

    // TODO: 调用 FSR 3.1 API 进行超分辨率
    // 这是占位符实现，实际需要调用 FFX SDK 的接口
    //
    // FfxFsr3DispatchDescription dispatchDesc = {};
    // dispatchDesc.commandList = GetFfxCommandList(context);
    // dispatchDesc.color = GetFfxResource(input.colorTexture);
    // dispatchDesc.depth = GetFfxResource(input.depthTexture);
    // dispatchDesc.motionVectors = GetFfxResource(input.motionVectorTexture);
    // dispatchDesc.output = GetFfxResource(output.outputTarget);
    // dispatchDesc.jitterOffset.x = input.jitterX;
    // dispatchDesc.jitterOffset.y = input.jitterY;
    // dispatchDesc.motionVectorScale.x = m_renderWidth;
    // dispatchDesc.motionVectorScale.y = m_renderHeight;
    // dispatchDesc.resetAccumulation = input.resetAccumulation || m_needReset;
    // dispatchDesc.enableSharpening = output.sharpnessEnabled;
    // dispatchDesc.sharpness = output.sharpness;
    //
    // FfxErrorCode errorCode = ffxFsr3ContextDispatch(m_fsrContext, &dispatchDesc);
    // if (errorCode != FFX_OK) {
    //     return false;
    // }

    m_needReset = false;
    m_frameIndex++;

    return true;
}

bool UpscalerFSR::SetQualityMode(UpscalerQuality quality) {
    if (!IsQualityModeSupported(quality)) {
        return false;
    }

    if (m_quality != quality) {
        m_quality = quality;
        m_needReset = true;
    }

    return true;
}

bool UpscalerFSR::SetRenderResolution(uint32_t width, uint32_t height) {
    if (width < FSR_MIN_RENDER_WIDTH || height < FSR_MIN_RENDER_HEIGHT) {
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

bool UpscalerFSR::SetDisplayResolution(uint32_t width, uint32_t height) {
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

void UpscalerFSR::GetRecommendedRenderResolution(UpscalerQuality quality,
                                                   uint32_t displayWidth,
                                                   uint32_t displayHeight,
                                                   uint32_t& outWidth,
                                                   uint32_t& outHeight) const {
    UpscalerHelper::CalculateRenderResolution(quality, displayWidth, displayHeight,
                                               outWidth, outHeight);
}

UpscalerInfo UpscalerFSR::GetInfo() const {
    UpscalerInfo info;
    info.technology = UpscalerTechnology::FSR;
    info.name = "AMD FidelityFX Super Resolution";
    info.version = "3.1.6";
    info.supportedQualities = {
        UpscalerQuality::UltraQuality,
        UpscalerQuality::Quality,
        UpscalerQuality::Balanced,
        UpscalerQuality::Performance,
        UpscalerQuality::UltraPerformance
    };
    info.requiresMotionVectors = true;
    info.requiresDepth = true;
    info.requiresExposure = false;
    info.requiresNormal = false;
    info.minRenderWidth = FSR_MIN_RENDER_WIDTH;
    info.minRenderHeight = FSR_MIN_RENDER_HEIGHT;
    return info;
}

bool UpscalerFSR::IsQualityModeSupported(UpscalerQuality quality) const {
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

bool UpscalerFSR::OnResize(uint32_t newWidth, uint32_t newHeight) {
    return SetDisplayResolution(newWidth, newHeight);
}

void UpscalerFSR::ReleaseResources() {
    if (m_resources) {
        m_resources->Release();
    }
    m_needReset = true;
}

std::string UpscalerFSR::GetDebugInfo() const {
    std::string info = "FSR 3.1 Upscaler:\n";
    info += "  Initialized: " + std::string(m_initialized ? "Yes" : "No") + "\n";
    info += "  Render Resolution: " + std::to_string(m_renderWidth) + "x" +
            std::to_string(m_renderHeight) + "\n";
    info += "  Display Resolution: " + std::to_string(m_displayWidth) + "x" +
            std::to_string(m_displayHeight) + "\n";
    info += "  Quality Mode: " + UpscalerHelper::GetQualityName(m_quality) + "\n";
    info += "  Frame Index: " + std::to_string(m_frameIndex) + "\n";
    info += "  Jitter: (" + std::to_string(m_jitterX) + ", " +
            std::to_string(m_jitterY) + ")\n";
    return info;
}

void UpscalerFSR::ResetHistory() {
    m_needReset = true;
    m_jitterIndex = 0;
    m_frameIndex = 0;
}

bool UpscalerFSR::CreateFSRContext() {
#if defined(PRISMA_ENABLE_UPSCALER_FSR)
    // TODO: 初始化 FFX FSR 3.1 上下文
    //
    // FfxFsr3ContextDescription contextDesc = {};
    // contextDesc.backendInterface = GetFfxBackendInterface();
    // contextDesc.maxRenderSize.width = m_renderWidth;
    // contextDesc.maxRenderSize.height = m_renderHeight;
    // contextDesc.displaySize.width = m_displayWidth;
    // contextDesc.displaySize.height = m_displayHeight;
    // contextDesc.flags = FFX_FSR3_ENABLE_AUTO_EXPOSURE;
    // if (m_enableHDR) {
    //     contextDesc.flags |= FFX_FSR3_ENABLE_HDR;
    // }
    //
    // FfxErrorCode errorCode = ffxFsr3ContextCreate(&m_fsrContext, &contextDesc);
    // return errorCode == FFX_OK;

    // 占位符：暂时返回 true
    return true;
#else
    return false;
#endif
}

void UpscalerFSR::DestroyFSRContext() {
#if defined(PRISMA_ENABLE_UPSCALER_FSR)
    // TODO: 销毁 FFX FSR 3.1 上下文
    // if (m_fsrContext) {
    //     ffxFsr3ContextDestroy(m_fsrContext);
    //     m_fsrContext = nullptr;
    // }
#endif
}

bool UpscalerFSR::CreateShaders() {
    // TODO: 编译和加载 FSR 着色器
    // FSR 着色器来自 FidelityFX SDK
    return true;
}

void UpscalerFSR::ReleaseShaders() {
    // TODO: 释放 FSR 着色器资源
}

UpscalerFSR::FSRQualityMode UpscalerFSR::GetFSRQualityMode(UpscalerQuality quality) const {
    switch (quality) {
        case UpscalerQuality::None:
        case UpscalerQuality::UltraQuality:
            return FSRQualityMode::Quality;  // FSR 没有专门的 Ultra Quality 模式，使用 Quality
        case UpscalerQuality::Quality:
            return FSRQualityMode::Quality;
        case UpscalerQuality::Balanced:
            return FSRQualityMode::Balanced;
        case UpscalerQuality::Performance:
            return FSRQualityMode::Performance;
        case UpscalerQuality::UltraPerformance:
            return FSRQualityMode::UltraPerformance;
        default:
            return FSRQualityMode::Quality;
    }
}

} // namespace PrismaEngine::Graphic
