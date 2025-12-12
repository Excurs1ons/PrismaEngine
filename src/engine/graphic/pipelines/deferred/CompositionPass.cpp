#include "CompositionPass.h"
#include "Logger.h"
#include <chrono>

namespace Engine {
namespace Graphic {
namespace Pipelines {
namespace Deferred {

CompositionPass::CompositionPass()
{
    LOG_DEBUG("CompositionPass", "合成通道构造函数被调用");
    m_stats = {};
}

CompositionPass::~CompositionPass()
{
    LOG_DEBUG("CompositionPass", "合成通道析构函数被调用");
}

void CompositionPass::Execute(RenderCommandContext* context)
{
    if (!context || !m_renderTarget || !m_lightingBuffer) {
        LOG_ERROR("CompositionPass", "无效的上下文、渲染目标或光照缓冲区");
        return;
    }

    LOG_DEBUG("CompositionPass", "开始执行合成通道");
    m_stats = {};
    m_stats.postProcessEffects = 0;

    auto startTime = std::chrono::high_resolution_clock::now();

    try {
        // 设置渲染目标
        context->SetRenderTargets(&m_renderTarget, 1, nullptr);

        // 设置视口
        context->SetViewport(0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height));

        // 清除渲染目标为黑色
        context->ClearRenderTarget(m_renderTarget, 0.0f, 0.0f, 0.0f, 1.0f);

        // 绑定光照缓冲区作为纹理
        context->SetShaderResource("LightingBuffer", m_lightingBuffer);

        // 绑定其他可选缓冲区
        if (m_aoBuffer) {
            context->SetShaderResource("AOBuffer", m_aoBuffer);
        }
        if (m_bloomBuffer) {
            context->SetShaderResource("BloomBuffer", m_bloomBuffer);
        }

        // 1. 首先渲染基础图像（光照结果）
        RenderFullScreenQuad(context);

        // 2. 应用后处理效果
        if (m_postProcessSettings.toneMapping) {
            ApplyToneMapping(context);
            m_stats.postProcessEffects++;
        }

        if (m_postProcessSettings.gammaCorrection) {
            // Gamma correction can be combined with tone mapping
        }

        if (m_postProcessSettings.fxaa) {
            ApplyFXAA(context);
            m_stats.postProcessEffects++;
        }

        if (m_postProcessSettings.smaa) {
            ApplySMAA(context);
            m_stats.postProcessEffects++;
        }

        if (m_postProcessSettings.bloom) {
            ApplyBloom(context);
            m_stats.postProcessEffects++;
        }

        if (m_postProcessSettings.ssr) {
            ApplySSR(context);
            m_stats.postProcessEffects++;
        }

        if (m_postProcessSettings.ssao) {
            ApplySSAO(context);
            m_stats.postProcessEffects++;
        }

        if (m_postProcessSettings.depthOfField) {
            ApplyDepthOfField(context);
            m_stats.postProcessEffects++;
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        m_stats.renderTime = duration.count() / 1000.0f; // Convert to milliseconds

        LOG_DEBUG("CompositionPass", "合成通道完成 - 后处理效果: {0}, 耗时: {1}ms",
                  m_stats.postProcessEffects, m_stats.renderTime);
    }
    catch (const std::exception& e) {
        LOG_ERROR("CompositionPass", "执行异常: {0}", e.what());
    }
}

void CompositionPass::SetRenderTarget(void* renderTarget)
{
    m_renderTarget = renderTarget;
    LOG_DEBUG("CompositionPass", "设置渲染目标: 0x{0:x}",
              reinterpret_cast<uintptr_t>(renderTarget));
}

void CompositionPass::ClearRenderTarget(float r, float g, float b, float a)
{
    if (m_renderTarget) {
        LOG_DEBUG("CompositionPass", "清除渲染目标: ({0}, {1}, {2}, {3})", r, g, b, a);
        // TODO: 实现渲染目标清除
    }
}

void CompositionPass::SetViewport(uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;
    LOG_DEBUG("CompositionPass", "设置视口: {0}x{1}", width, height);
}

void CompositionPass::SetLightingBuffer(void* lightingBuffer)
{
    m_lightingBuffer = lightingBuffer;
    LOG_DEBUG("CompositionPass", "设置光照缓冲区: 0x{0:x}",
              reinterpret_cast<uintptr_t>(lightingBuffer));
}

void CompositionPass::SetAOBuffer(void* aoBuffer)
{
    m_aoBuffer = aoBuffer;
    LOG_DEBUG("CompositionPass", "设置AO缓冲区: 0x{0:x}",
              reinterpret_cast<uintptr_t>(aoBuffer));
}

void CompositionPass::SetBloomBuffer(void* bloomBuffer)
{
    m_bloomBuffer = bloomBuffer;
    LOG_DEBUG("CompositionPass", "设置泛光缓冲区: 0x{0:x}",
              reinterpret_cast<uintptr_t>(bloomBuffer));
}

void CompositionPass::SetPostProcessEffect(PostProcessEffect effect, bool enable)
{
    switch (effect) {
    case PostProcessEffect::ToneMapping:
        m_postProcessSettings.toneMapping = enable;
        break;
    case PostProcessEffect::GammaCorrection:
        m_postProcessSettings.gammaCorrection = enable;
        break;
    case PostProcessEffect::FXAA:
        m_postProcessSettings.fxaa = enable;
        break;
    case PostProcessEffect::SMAA:
        m_postProcessSettings.smaa = enable;
        break;
    case PostProcessEffect::Bloom:
        m_postProcessSettings.bloom = enable;
        break;
    case PostProcessEffect::SSR:
        m_postProcessSettings.ssr = enable;
        break;
    case PostProcessEffect::SSAO:
        m_postProcessSettings.ssao = enable;
        break;
    case PostProcessEffect::DepthOfField:
        m_postProcessSettings.depthOfField = enable;
        break;
    }

    LOG_DEBUG("CompositionPass", "后处理效果 {0}: {1}",
              static_cast<uint32_t>(effect), enable ? "启用" : "禁用");
}

bool CompositionPass::IsPostProcessEffectEnabled(PostProcessEffect effect) const
{
    switch (effect) {
    case PostProcessEffect::ToneMapping:
        return m_postProcessSettings.toneMapping;
    case PostProcessEffect::GammaCorrection:
        return m_postProcessSettings.gammaCorrection;
    case PostProcessEffect::FXAA:
        return m_postProcessSettings.fxaa;
    case PostProcessEffect::SMAA:
        return m_postProcessSettings.smaa;
    case PostProcessEffect::Bloom:
        return m_postProcessSettings.bloom;
    case PostProcessEffect::SSR:
        return m_postProcessSettings.ssr;
    case PostProcessEffect::SSAO:
        return m_postProcessSettings.ssao;
    case PostProcessEffect::DepthOfField:
        return m_postProcessSettings.depthOfField;
    default:
        return false;
    }
}

void CompositionPass::SetToneMappingParams(float exposure, float gamma)
{
    m_toneMappingParams.exposure = exposure;
    m_toneMappingParams.gamma = gamma;
    LOG_DEBUG("CompositionPass", "色调映射参数: 曝光={0}, Gamma={1}", exposure, gamma);
}

void CompositionPass::SetFXAAParams(float edgeThresholdMin, float edgeThresholdMax)
{
    m_fxaaParams.edgeThresholdMin = edgeThresholdMin;
    m_fxaaParams.edgeThresholdMax = edgeThresholdMax;
    LOG_DEBUG("CompositionPass", "FXAA参数: 最小边缘阈值={0}, 最大边缘阈值={1}",
              edgeThresholdMin, edgeThresholdMax);
}

void CompositionPass::SetSSAOParams(float radius, float bias, float power)
{
    m_ssaoParams.radius = radius;
    m_ssaoParams.bias = bias;
    m_ssaoParams.power = power;
    LOG_DEBUG("CompositionPass", "SSAO参数: 半径={0}, 偏差={1}, 强度={2}",
              radius, bias, power);
}

void CompositionPass::RenderFullScreenQuad(RenderCommandContext* context)
{
    // TODO: 渲染全屏四边形
    // 这需要使用预先准备的顶点缓冲区或使用顶点着色器生成
}

void CompositionPass::ApplyToneMapping(RenderCommandContext* context)
{
    LOG_DEBUG("CompositionPass", "应用色调映射");

    // 设置色调映射参数
    context->SetConstantBuffer("ToneMappingExposure", &m_toneMappingParams.exposure, sizeof(float));
    context->SetConstantBuffer("ToneMappingGamma", &m_toneMappingParams.gamma, sizeof(float));

    // TODO: 切换到色调映射着色器并渲染
}

void CompositionPass::ApplyFXAA(RenderCommandContext* context)
{
    LOG_DEBUG("CompositionPass", "应用FXAA抗锯齿");

    // 设置FXAA参数
    context->SetConstantBuffer("FXAAEdgeThresholdMin", &m_fxaaParams.edgeThresholdMin, sizeof(float));
    context->SetConstantBuffer("FXAAEdgeThresholdMax", &m_fxaaParams.edgeThresholdMax, sizeof(float));

    // TODO: 切换到FXAA着色器并渲染
}

void CompositionPass::ApplySMAA(RenderCommandContext* context)
{
    LOG_DEBUG("CompositionPass", "应用SMAA抗锯齿");

    // TODO: 切换到SMAA着色器并渲染
}

void CompositionPass::ApplyBloom(RenderCommandContext* context)
{
    LOG_DEBUG("CompositionPass", "应用泛光效果");

    // TODO: 切换到泛光着色器并渲染
}

void CompositionPass::ApplySSR(RenderCommandContext* context)
{
    LOG_DEBUG("CompositionPass", "应用SSR效果");

    // TODO: 切换到SSR着色器并渲染
}

void CompositionPass::ApplySSAO(RenderCommandContext* context)
{
    LOG_DEBUG("CompositionPass", "应用SSAO效果");

    // 设置SSAO参数
    context->SetConstantBuffer("SSAORadius", &m_ssaoParams.radius, sizeof(float));
    context->SetConstantBuffer("SSAOBias", &m_ssaoParams.bias, sizeof(float));
    context->SetConstantBuffer("SSAOPower", &m_ssaoParams.power, sizeof(float));

    // TODO: 切换到SSAO着色器并渲染
}

void CompositionPass::ApplyDepthOfField(RenderCommandContext* context)
{
    LOG_DEBUG("CompositionPass", "应用景深效果");

    // TODO: 切换到景深着色器并渲染
}

} // namespace Deferred
} // namespace Pipelines
} // namespace Graphic
} // namespace Engine