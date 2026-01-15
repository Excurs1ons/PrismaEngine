#include "UpscalerPass.h"
#include "UpscalerManager.h"
#include "interfaces/IRenderTarget.h"
#include "interfaces/IDepthStencil.h"
#include <cstring>

namespace PrismaEngine::Graphic {

// 默认渲染分辨率
static constexpr uint32_t DEFAULT_RENDER_WIDTH = 1920;
static constexpr uint32_t DEFAULT_RENDER_HEIGHT = 1080;

// 默认显示分辨率
static constexpr uint32_t DEFAULT_DISPLAY_WIDTH = 1920;
static constexpr uint32_t DEFAULT_DISPLAY_HEIGHT = 1080;


// ========== UpscalerPass Implementation ==========

UpscalerPass::UpscalerPass()
    : m_viewMatrix(1.0f)
    , m_projMatrix(1.0f)
    , m_viewProjMatrix(1.0f)
    , m_prevViewProjMatrix(1.0f)
{
    // 尝试初始化默认超分辨率器
    InitializeUpscaler();
}

UpscalerPass::~UpscalerPass() {
    ReleaseUpscaler();
}

void UpscalerPass::SetRenderTarget(IRenderTarget* renderTarget) {
    m_outputTarget = renderTarget;
}

void UpscalerPass::SetDepthStencil(IDepthStencil* depthStencil) {
    // UpscalerPass 不直接使用深度模板
    // 深度通过 SetDepthInput 设置
}

void UpscalerPass::SetViewport(uint32_t width, uint32_t height) {
    m_displayWidth = width;
    m_displayHeight = height;

    // 根据质量模式计算渲染分辨率
    if (m_upscaler && m_upscaler->IsInitialized()) {
        uint32_t renderWidth, renderHeight;
        m_upscaler->GetRecommendedRenderResolution(
            m_quality,
            m_displayWidth,
            m_displayHeight,
            renderWidth,
            renderHeight
        );
        m_renderWidth = renderWidth;
        m_renderHeight = renderHeight;

        // 更新超分辨率器的显示分辨率
        m_upscaler->SetDisplayResolution(m_displayWidth, m_displayHeight);
        m_upscaler->SetRenderResolution(m_renderWidth, m_renderHeight);
    }
}

void UpscalerPass::Update(float deltaTime) {
    // 生成下一帧的抖动偏移
    UpscalerHelper::GenerateHaltonSequence(m_jitterIndex, m_jitterX, m_jitterY);
    m_jitterIndex = (m_jitterIndex + 1) % 16;
}

void UpscalerPass::Execute(const PassExecutionContext& context) {
    if (!m_enabled || !m_upscaler) {
        return;
    }

    // 验证输入
    if (!m_colorInput || !m_outputTarget) {
        return;
    }

    // 准备输入描述
    UpscalerInputDesc inputDesc;
    inputDesc.colorTexture = m_colorInput;  // IRenderTarget 继承自 ITexture
    inputDesc.depthTexture = m_depthInput;
    inputDesc.motionVectorTexture = m_motionVectors;
    inputDesc.normalTexture = m_normalInput;
    inputDesc.jitterX = m_jitterX;
    inputDesc.jitterY = m_jitterY;
    inputDesc.deltaTime = context.sceneData->time.deltaTime;

    // 相机信息
    inputDesc.camera.view = m_viewMatrix;
    inputDesc.camera.projection = m_projMatrix;
    inputDesc.camera.viewProjection = m_viewProjMatrix;
    inputDesc.camera.prevViewProjection = m_prevViewProjMatrix;

    // 准备输出描述
    UpscalerOutputDesc outputDesc;
    outputDesc.outputTarget = m_outputTarget;
    outputDesc.outputWidth = m_displayWidth;
    outputDesc.outputHeight = m_displayHeight;
    outputDesc.sharpnessEnabled = true;
    outputDesc.sharpness = 0.5f;

    // 执行超分辨率
    context.deviceContext->BeginDebugMarker("UpscalerPass");

    bool success = m_upscaler->Upscale(context.deviceContext, inputDesc, outputDesc);

    context.deviceContext->EndDebugMarker();

    // 更新上一帧的投影矩阵
    m_prevViewProjMatrix = m_viewProjMatrix;
}

bool UpscalerPass::SetUpscaler(UpscalerTechnology technology) {
    auto& manager = UpscalerManager::Instance();

    if (!manager.IsTechnologyAvailable(technology)) {
        return false;
    }

    // 释放旧超分器资源
    if (m_upscaler && m_upscaler->IsInitialized()) {
        m_upscaler->ReleaseResources();
    }

    // 获取超分辨率器
    IUpscaler* upscaler = manager.GetUpscaler(technology);
    if (!upscaler) {
        return false;
    }

    // 初始化超分辨率器
    UpscalerInitDesc desc;
    desc.renderWidth = m_renderWidth > 0 ? m_renderWidth : DEFAULT_RENDER_WIDTH;
    desc.renderHeight = m_renderHeight > 0 ? m_renderHeight : DEFAULT_RENDER_HEIGHT;
    desc.displayWidth = m_displayWidth > 0 ? m_displayWidth : DEFAULT_DISPLAY_WIDTH;
    desc.displayHeight = m_displayHeight > 0 ? m_displayHeight : DEFAULT_DISPLAY_HEIGHT;
    desc.quality = m_quality;
    desc.maxFramesInFlight = 2;

    if (!upscaler->Initialize(desc)) {
        return false;
    }

    // 重置历史
    upscaler->ResetHistory();

    m_upscaler = upscaler;
    m_currentTechnology = technology;

    return true;
}

UpscalerTechnology UpscalerPass::GetCurrentTechnology() const {
    return m_currentTechnology;
}

void UpscalerPass::SetQualityMode(UpscalerQuality quality) {
    if (m_upscaler && m_upscaler->IsQualityModeSupported(quality)) {
        m_upscaler->SetQualityMode(quality);
        m_quality = quality;

        // 更新渲染分辨率
        SetViewport(m_displayWidth, m_displayHeight);
    }
}

void UpscalerPass::UpdateCameraInfo(const PrismaMath::mat4& view,
                                     const PrismaMath::mat4& projection,
                                     const PrismaMath::mat4& prevViewProjection) {
    m_viewMatrix = view;
    m_projMatrix = projection;
    m_viewProjMatrix = projection * view;
    m_prevViewProjMatrix = prevViewProjection;
}

void UpscalerPass::GetJitterOffset(float& x, float& y) const {
    x = m_jitterX;
    y = m_jitterY;
}

bool UpscalerPass::InitializeUpscaler() {
    auto& manager = UpscalerManager::Instance();

    if (!manager.IsInitialized()) {
        // 如果管理器未初始化，暂时返回 false
        // 管理器会在引擎初始化时调用 Initialize
        return false;
    }

    // 使用默认技术
    auto defaultTech = UpscalerManager::GetDefaultTechnology();
    return SetUpscaler(defaultTech);
}

void UpscalerPass::ReleaseUpscaler() {
    m_upscaler = nullptr;
    m_currentTechnology = UpscalerTechnology::None;
}

} // namespace PrismaEngine::Graphic
