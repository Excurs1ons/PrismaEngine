#include "CompositionPass.h"
#include "app/Engine.h"
#include "graphic/interfaces/IResourceManager.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/ISwapChain.h"
#include "graphic/interfaces/ITexture.h"
#include "graphic/RenderResourceManager.h"
#include "Logger.h"

namespace Prisma::Graphic {

CompositionPass::CompositionPass()
    : LogicalPass("CompositionPass")
    , m_inputTexture(nullptr)
    , m_lightingBuffer(nullptr)
    , m_aoBuffer(nullptr)
    , m_bloomBuffer(nullptr)
{
    m_priority = 900;
}

void CompositionPass::Update(Timestep ts) {
    UpdateTime(ts);
}

void CompositionPass::Execute(const PassExecutionContext& context) {
    if (!context.deviceContext) return;
    m_stats = {};
    m_stats.postProcessEffects = 0;

    context.deviceContext->SetViewport(0.0f, 0.0f,
        static_cast<float>(context.sceneData->viewport.width),
        static_cast<float>(context.sceneData->viewport.height));

    if (m_postProcessSettings.toneMapping) m_stats.postProcessEffects++;
    if (m_postProcessSettings.gammaCorrection) m_stats.postProcessEffects++;
    if (m_postProcessSettings.fxaa) m_stats.postProcessEffects++;
    if (m_postProcessSettings.smaa) m_stats.postProcessEffects++;
    if (m_postProcessSettings.bloom) m_stats.postProcessEffects++;
    if (m_postProcessSettings.ssr) m_stats.postProcessEffects++;
    if (m_postProcessSettings.ssao) m_stats.postProcessEffects++;
    if (m_postProcessSettings.depthOfField) m_stats.postProcessEffects++;
}

void CompositionPass::Execute(ICommandBuffer* cmd, IRenderDevice* device) {
    if (!cmd || !device) return;
    if (!EnsureDefaultPipeline(device)) return;

    m_stats = {};
    m_stats.postProcessEffects = 0;
    if (m_postProcessSettings.toneMapping) m_stats.postProcessEffects++;
    if (m_postProcessSettings.gammaCorrection) m_stats.postProcessEffects++;

    cmd->SetPipelineState(m_pipelineState.get());

    float w = 1.0f, h = 1.0f;
    auto* swapChain = device->GetSwapChain();
    if (swapChain) {
        w = static_cast<float>(swapChain->GetWidth());
        h = static_cast<float>(swapChain->GetHeight());
    }
    cmd->SetViewport(Viewport{0.0f, 0.0f, w, h, 0.0f, 1.0f});
    cmd->SetScissorRect(Rect{0, 0, static_cast<int>(w), static_cast<int>(h)});

    struct PushData {
        float exposure;
        float gamma;
    };
    PushData pd{};
    pd.exposure = m_toneMappingParams.exposure;
    pd.gamma = m_toneMappingParams.gamma;

    cmd->PushConstants(ShaderType::Vertex, &pd, sizeof(pd));
    cmd->PushConstants(ShaderType::Pixel, &pd, sizeof(pd));

    if (m_inputTexture) {
    }

    cmd->Draw(3, 1);
}

void CompositionPass::SetPostProcessEffect(PostProcessEffect effect, bool enable) {
    switch (effect) {
    case PostProcessEffect::ToneMapping:
        m_postProcessSettings.toneMapping = enable; break;
    case PostProcessEffect::GammaCorrection:
        m_postProcessSettings.gammaCorrection = enable; break;
    case PostProcessEffect::FXAA:
        m_postProcessSettings.fxaa = enable; break;
    case PostProcessEffect::SMAA:
        m_postProcessSettings.smaa = enable; break;
    case PostProcessEffect::Bloom:
        m_postProcessSettings.bloom = enable; break;
    case PostProcessEffect::SSR:
        m_postProcessSettings.ssr = enable; break;
    case PostProcessEffect::SSAO:
        m_postProcessSettings.ssao = enable; break;
    case PostProcessEffect::DepthOfField:
        m_postProcessSettings.depthOfField = enable; break;
    case PostProcessEffect::None: break;
    }
}

bool CompositionPass::IsPostProcessEffectEnabled(PostProcessEffect effect) const {
    switch (effect) {
    case PostProcessEffect::ToneMapping:    return m_postProcessSettings.toneMapping;
    case PostProcessEffect::GammaCorrection: return m_postProcessSettings.gammaCorrection;
    case PostProcessEffect::FXAA:           return m_postProcessSettings.fxaa;
    case PostProcessEffect::SMAA:           return m_postProcessSettings.smaa;
    case PostProcessEffect::Bloom:          return m_postProcessSettings.bloom;
    case PostProcessEffect::SSR:            return m_postProcessSettings.ssr;
    case PostProcessEffect::SSAO:           return m_postProcessSettings.ssao;
    case PostProcessEffect::DepthOfField:  return m_postProcessSettings.depthOfField;
    default: return false;
    }
}

void CompositionPass::SetToneMappingParams(float exposure, float gamma) {
    m_toneMappingParams.exposure = exposure;
    m_toneMappingParams.gamma = gamma;
}

void CompositionPass::SetFXAAParams(float edgeThresholdMin, float edgeThresholdMax) {
    m_fxaaParams.edgeThresholdMin = edgeThresholdMin;
    m_fxaaParams.edgeThresholdMax = edgeThresholdMax;
}

void CompositionPass::SetSSAOParams(float radius, float bias, float power) {
    m_ssaoParams.radius = radius;
    m_ssaoParams.bias = bias;
    m_ssaoParams.power = power;
}

bool CompositionPass::EnsureDefaultPipeline(IRenderDevice* device) {
    if (m_pipelineState) return true;
    if (!device || !device->GetResourceFactory()) return false;

    auto* resourceManager = Engine::Get().GetRenderResourceManager();
    if (!resourceManager) return false;

    m_vertexShader = resourceManager->LoadShaderSync(
        "assets/shaders/deferred_fullscreen.vert.spv", "main");
    m_fragmentShader = resourceManager->LoadShaderSync(
        "assets/shaders/deferred_composite.frag.spv", "main");

    if (!m_vertexShader || !m_fragmentShader) {
        LOG_ERROR("CompositionPass", "shader loading failed");
        return false;
    }

    auto pso = device->GetResourceFactory()->CreatePipelineStateImpl();
    pso->SetShader(ShaderType::Vertex, m_vertexShader);
    pso->SetShader(ShaderType::Pixel, m_fragmentShader);
    pso->SetPrimitiveTopology(PrimitiveTopology::TriangleList);

    RasterizerState rs;
    rs.cullMode = CullMode::None;
    pso->SetRasterizerState(rs);

    DepthStencilState ds;
    ds.depthEnable = false;
    ds.depthWriteEnable = false;
    pso->SetDepthStencilState(ds);

    BlendState bs;
    bs.blendEnable = false;
    pso->SetBlendState(bs);

    if (!pso->Create(device)) {
        LOG_ERROR("CompositionPass", "PSO creation failed: {}", pso->GetErrors());
        return false;
    }
    m_pipelineState = std::shared_ptr<IPipelineState>(std::move(pso));
    LOG_INFO("CompositionPass", "PSO created");
    return true;
}

} // namespace Prisma::Graphic
