#include "MotionVectorPass.h"
#include "interfaces/IRenderDevice.h"
#include "interfaces/ITexture.h"
#include "interfaces/IBuffer.h"
#include "interfaces/IShader.h"
#include <cstring>

namespace PrismaEngine::Graphic {

// 运动矢量纹理格式 (RG16_FLOAT = 2通道 16位浮点)
static constexpr TextureFormat MOTION_VECTOR_FORMAT = TextureFormat::RG16_Float;


// ========== MotionVectorPass Implementation ==========

MotionVectorPass::MotionVectorPass()
    : m_invView(1.0f)
    , m_invProj(1.0f)
    , m_prevViewProj(1.0f)
{
}

MotionVectorPass::~MotionVectorPass() {
    ReleaseResources();
}

void MotionVectorPass::SetRenderTarget(IRenderTarget* renderTarget) {
    // 将 IRenderTarget* 转换为 ITextureRenderTarget*
    m_motionVectorOutput = dynamic_cast<ITextureRenderTarget*>(renderTarget);
}

void MotionVectorPass::SetDepthStencil(IDepthStencil* depthStencil) {
    m_currentDepth = depthStencil;
}

void MotionVectorPass::SetViewport(uint32_t width, uint32_t height) {
    m_width = width;
    m_height = height;

    // 重新创建运动矢量输出
    if (m_width > 0 && m_height > 0) {
        ReleaseResources();
        InitializeResources();
    }
}

void MotionVectorPass::Update(float deltaTime) {
    // 更新常量缓冲区数据
    if (m_cameraConstants) {
        CameraConstants constants;
        constants.inverseViewProjection = m_invView * m_invProj;
        constants.previousViewProjection = m_prevViewProj;
        constants.resolution = PrismaMath::vec2(static_cast<float>(m_width),
                                                 static_cast<float>(m_height));
        constants.padding0 = 0.0f;
        constants.padding1 = 0.0f;

        // TODO: 更新常量缓冲区
        // auto context = m_device->GetDeviceContext();
        // context->UpdateBuffer(m_cameraConstants, &constants, sizeof(constants));
    }
}

void MotionVectorPass::Execute(const PassExecutionContext& context) {
    if (!m_enabled || !m_motionVectorOutput) {
        return;
    }

    context.deviceContext->BeginDebugMarker("MotionVectorPass");

    // TODO: 实现运动矢量计算
    //
    // 1. 设置渲染目标为运动矢量输出
    // context.deviceContext->SetRenderTarget(m_motionVectorOutput);
    //
    // 2. 清除运动矢量为零
    // float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    // context.deviceContext->ClearRenderTarget(m_motionVectorOutput, clearColor);
    //
    // 3. 设置着色器和常量缓冲区
    // context.deviceContext->SetPipelineState(m_pipelineState);
    // context.deviceContext->SetConstantBuffer(m_cameraConstants, 0);
    //
    // 4. 绑定深度和 G-Buffer 作为输入
    // context.deviceContext->SetTexture(m_currentDepth, 0);
    // if (m_gBuffer) {
    //     context.deviceContext->SetTexture(m_gBuffer->GetTexture(GBufferTarget::Position), 1);
    // }
    //
    // 5. 绘制全屏四边形
    // context.deviceContext->Draw(3, 0);  // 全屏三角形

    context.deviceContext->EndDebugMarker();
}

void MotionVectorPass::UpdateCameraInfo(const PrismaMath::mat4& view,
                                        const PrismaMath::mat4& projection,
                                        const PrismaMath::mat4& prevViewProjection) {
    m_invView = PrismaMath::inverse(view);
    m_invProj = PrismaMath::inverse(projection);
    m_prevViewProj = prevViewProjection;
}

bool MotionVectorPass::InitializeResources() {
    // TODO: 创建运动矢量输出渲染目标
    //
    // TextureDesc texDesc;
    // texDesc.type = TextureType::Texture2D;
    // texDesc.format = MOTION_VECTOR_FORMAT;
    // texDesc.width = m_width;
    // texDesc.height = m_height;
    // texDesc.mipLevels = 1;
    // texDesc.allowRenderTarget = true;
    // texDesc.allowShaderResource = true;
    //
    // m_motionVectorOutput = m_device->CreateTexture(texDesc);

    // TODO: 创建相机常量缓冲区
    //
    // BufferDesc bufferDesc;
    // bufferDesc.type = BufferType::Constant;
    // bufferDesc.size = sizeof(CameraConstants);
    // bufferDesc.usage = BufferUsage::Dynamic;
    //
    // m_cameraConstants = m_device->CreateBuffer(bufferDesc);

    // 创建着色器
    if (!CreateShaders()) {
        return false;
    }

    return true;
}

void MotionVectorPass::ReleaseResources() {
    // TODO: 释放资源
    // if (m_motionVectorOutput) {
    //     m_motionVectorOutput->Release();
    //     m_motionVectorOutput = nullptr;
    // }
    // if (m_cameraConstants) {
    //     m_cameraConstants->Release();
    //     m_cameraConstants = nullptr;
    // }
}

bool MotionVectorPass::CreateShaders() {
    // TODO: 编译和加载运动矢量着色器
    //
    // 着色器路径：
    // - HLSL: resources/common/shaders/hlsl/MotionVector.hlsl
    // - GLSL: resources/common/shaders/glsl/MotionVector.vert/.frag
    //
    // ShaderDesc shaderDesc;
    // shaderDesc.type = ShaderType::Pixel;
    // shaderDesc.language = ShaderLanguage::HLSL;
    // shaderDesc.entryPoint = "main";
    //
    // m_motionVectorShader = m_device->CreateShader(shaderDesc);

    // 占位符：暂时返回 true
    return true;
}

} // namespace PrismaEngine::Graphic
