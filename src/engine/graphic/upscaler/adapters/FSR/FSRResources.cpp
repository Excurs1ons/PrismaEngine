#include "FSRResources.h"
#include "interfaces/IRenderDevice.h"
#include "interfaces/ITexture.h"
#include "interfaces/IBuffer.h"
#include <cstring>

namespace PrismaEngine::Graphic {

// ========== FSR 资源格式配置 ==========

// 颜色纹理格式 (支持 HDR)
static constexpr TextureFormat FSR_COLOR_FORMAT = TextureFormat::RGBA16_Float;

// 深度纹理格式
static constexpr TextureFormat FSR_DEPTH_FORMAT = TextureFormat::D32_Float;

// 运动矢量格式 (RG16_FLOAT = 2通道 16位浮点)
static constexpr TextureFormat FSR_MOTION_VECTOR_FORMAT = TextureFormat::RG16_Float;

// 曝光格式 (R32_FLOAT = 单通道 32位浮点)
static constexpr TextureFormat FSR_EXPOSURE_FORMAT = TextureFormat::R32_Float;

// 锁掩码格式 (R8_UNorm = 单通道 8位)
static constexpr TextureFormat FSR_LOCK_MASK_FORMAT = TextureFormat::R8_UNorm;

// 输出格式 (与颜色输入相同)
static constexpr TextureFormat FSR_OUTPUT_FORMAT = TextureFormat::RGBA16_Float;


// ========== FSRResources Implementation ==========

FSRResources::FSRResources() = default;

FSRResources::~FSRResources() {
    Release();
}

bool FSRResources::Initialize(IRenderDevice* device,
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

    // 创建缓冲区资源
    if (!CreateBuffers()) {
        Release();
        return false;
    }

    m_initialized = true;
    return true;
}

void FSRResources::Release() {
    if (!m_initialized) {
        return;
    }

    ReleaseTextures();
    ReleaseBuffers();

    m_device = nullptr;
    m_initialized = false;
}

ITexture* FSRResources::GetCurrentColorInput() const {
    return m_colorInput.get();
}

ITexture* FSRResources::GetDepthInput() const {
    return m_depthInput.get();
}

ITexture* FSRResources::GetMotionVectorInput() const {
    return m_motionVectors.get();
}

ITexture* FSRResources::GetExposureInput() const {
    return m_exposure.get();
}

ITexture* FSRResources::GetUpscaledOutput() const {
    return m_upscaledOutput.get();
}

ITexture* FSRResources::GetRCASOutput() const {
    return m_rcasOutput.get();
}

ITexture* FSRResources::GetHistoryColor() const {
    if (m_historyColor.empty() || m_currentFrameIndex >= m_historyColor.size()) {
        return nullptr;
    }
    return m_historyColor[m_currentFrameIndex].get();
}

ITexture* FSRResources::GetHistoryDepth() const {
    if (m_historyDepth.empty() || m_currentFrameIndex >= m_historyDepth.size()) {
        return nullptr;
    }
    return m_historyDepth[m_currentFrameIndex].get();
}

void FSRResources::PrepareNextFrame() {
    // 交换历史缓冲区索引
    m_currentFrameIndex = (m_currentFrameIndex + 1) % m_maxFramesInFlight;
}

IBuffer* FSRResources::GetConstantBuffer() const {
    return m_constantBuffer.get();
}

void FSRResources::UpdateConstantBuffer(const void* data, uint32_t size) {
    if (!m_constantBuffer || !data || size == 0) {
        return;
    }

    // TODO: 更新常量缓冲区数据
    // 这需要使用设备上下文来更新缓冲区内容
    //
    // auto context = m_device->GetDeviceContext();
    // context->UpdateBuffer(m_constantBuffer.get(), data, size);
}

bool FSRResources::CreateTextures() {
    // TODO: 创建 FSR 所需的所有纹理资源
    // 这些纹理需要通过 IRenderDevice 接口创建
    //
    // 示例代码（需要根据实际设备接口调整）：
    //
    // TextureDesc colorDesc;
    // colorDesc.type = TextureType::Texture2D;
    // colorDesc.format = FSR_COLOR_FORMAT;
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

bool FSRResources::CreateBuffers() {
    // TODO: 创建 FSR 常量缓冲区
    //
    // BufferDesc bufferDesc;
    // bufferDesc.type = BufferType::Constant;
    // bufferDesc.size = sizeof(FSRConstantData);
    // bufferDesc.usage = BufferUsage::Dynamic;
    //
    // m_constantBuffer = m_device->CreateBuffer(bufferDesc);
    // return m_constantBuffer != nullptr;

    // 占位符：暂时返回 true
    return true;
}

void FSRResources::ReleaseTextures() {
    m_colorInput.reset();
    m_depthInput.reset();
    m_motionVectors.reset();
    m_exposure.reset();
    m_autoExposure.reset();
    m_lockMask.reset();
    m_lockNewMask.reset();
    m_upscaledOutput.reset();
    m_rcasOutput.reset();
    m_historyColor.clear();
    m_historyDepth.clear();
}

void FSRResources::ReleaseBuffers() {
    m_constantBuffer.reset();
}

} // namespace PrismaEngine::Graphic
