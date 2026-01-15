#include "DLSSResources.h"
#include "interfaces/IRenderDevice.h"
#include "interfaces/ITexture.h"
#include "interfaces/IBuffer.h"
#include <cstring>

namespace PrismaEngine::Graphic {

// ========== DLSS 资源格式配置 ==========

// 颜色纹理格式 (支持 HDR)
static constexpr TextureFormat DLSS_COLOR_FORMAT = TextureFormat::RGBA16_Float;

// 深度纹理格式
static constexpr TextureFormat DLSS_DEPTH_FORMAT = TextureFormat::D32_Float;

// 运动矢量格式 (RG16_FLOAT = 2通道 16位浮点)
static constexpr TextureFormat DLSS_MOTION_VECTOR_FORMAT = TextureFormat::RG16_Float;

// 曝光格式 (R32_FLOAT = 单通道 32位浮点，DLSS 必需)
static constexpr TextureFormat DLSS_EXPOSURE_FORMAT = TextureFormat::R32_Float;

// 输出格式
static constexpr TextureFormat DLSS_OUTPUT_FORMAT = TextureFormat::RGBA16_Float;


// ========== DLSSResources Implementation ==========

DLSSResources::DLSSResources() = default;

DLSSResources::~DLSSResources() {
    Release();
}

bool DLSSResources::Initialize(IRenderDevice* device,
                                uint32_t renderWidth,
                                uint32_t renderHeight,
                                uint32_t displayWidth,
                                uint32_t displayHeight,
                                uint32_t maxFramesInFlight) {
    if (m_initialized) {
        return true;
    }

    if (!device) {
        return false;
    }

    m_device = device;
    m_renderWidth = renderWidth;
    m_renderHeight = renderHeight;
    m_displayWidth = displayWidth;
    m_displayHeight = displayHeight;
    m_maxFramesInFlight = maxFramesInFlight;
    m_currentFrameIndex = 0;

    // 创建纹理资源
    if (!CreateTextures()) {
        Release();
        return false;
    }

    m_initialized = true;
    return true;
}

void DLSSResources::Release() {
    if (!m_initialized) {
        return;
    }

    ReleaseTextures();

    m_device = nullptr;
    m_initialized = false;
}

ITexture* DLSSResources::GetCurrentColorInput() const {
    return m_colorInput.get();
}

ITexture* DLSSResources::GetDepthInput() const {
    return m_depthInput.get();
}

ITexture* DLSSResources::GetMotionVectorInput() const {
    return m_motionVectors.get();
}

ITexture* DLSSResources::GetExposureInput() const {
    return m_exposure.get();
}

ITexture* DLSSResources::GetUpscaledOutput() const {
    return m_upscaledOutput.get();
}

ITexture* DLSSResources::GetHistoryColor() const {
    if (m_historyColor.empty() || m_currentFrameIndex >= m_historyColor.size()) {
        return nullptr;
    }
    return m_historyColor[m_currentFrameIndex].get();
}

void DLSSResources::PrepareNextFrame() {
    // 交换历史缓冲区索引
    m_currentFrameIndex = (m_currentFrameIndex + 1) % m_maxFramesInFlight;
}

bool DLSSResources::CreateTextures() {
    // TODO: 创建 DLSS 所需的所有纹理资源
    //
    // 示例代码（需要根据实际设备接口调整）：
    //
    // // 颜色输入
    // TextureDesc colorDesc;
    // colorDesc.type = TextureType::Texture2D;
    // colorDesc.format = DLSS_COLOR_FORMAT;
    // colorDesc.width = m_renderWidth;
    // colorDesc.height = m_renderHeight;
    // colorDesc.mipLevels = 1;
    // colorDesc.allowRenderTarget = true;
    // colorDesc.allowShaderResource = true;
    // colorDesc.allowUnorderedAccess = true;
    //
    // m_colorInput = m_device->CreateTexture(colorDesc);
    // if (!m_colorInput) return false;

    // 占位符：暂时返回 true
    // 实际实现需要在图形设备层完成后补充
    return true;
}

void DLSSResources::ReleaseTextures() {
    m_colorInput.reset();
    m_depthInput.reset();
    m_motionVectors.reset();
    m_exposure.reset();
    m_upscaledOutput.reset();
    m_historyColor.clear();
}

} // namespace PrismaEngine::Graphic
