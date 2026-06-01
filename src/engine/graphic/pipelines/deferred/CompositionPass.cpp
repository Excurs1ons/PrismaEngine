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
    , m_outputTexture(nullptr)
    , m_lightingBuffer(nullptr)
    , m_aoBuffer(nullptr)
    , m_bloomBuffer(nullptr)
    , m_gbColor(nullptr)
    , m_gbNormal(nullptr)
    , m_gbDepth(nullptr)
    , m_gbPosition(nullptr)
{
    m_priority = 900;
}

bool CompositionPass::Setup(IRenderDevice* device) {
    if (!device) return false;

    auto* factory = device->GetResourceFactory();
    if (!factory) return false;

    // Create linear sampler for composite
    SamplerDesc linearDesc;
    linearDesc.filter = TextureFilter::Linear;
    linearDesc.addressU = TextureAddressMode::Clamp;
    linearDesc.addressV = TextureAddressMode::Clamp;
    linearDesc.addressW = TextureAddressMode::Clamp;
    m_linearSampler.reset(factory->CreateSamplerImpl(linearDesc).release());

    // Initialize sub-passes
    m_ssrPass = std::make_unique<SSRPass>();
    if (!m_ssrPass->Setup(device)) {
        LOG_WARN("CompositionPass", "SSR pass initialization failed, SSR disabled");
    }

    m_tonemapPass = std::make_unique<TonemappingPass>();
    if (!m_tonemapPass->Setup(device)) {
        LOG_WARN("CompositionPass", "Tonemapping pass initialization failed");
    }

    m_fogPass = std::make_unique<FogPass>();
    if (!m_fogPass->Setup(device)) {
        LOG_WARN("CompositionPass", "Fog pass initialization failed, fog disabled");
    }

    // Ensure the default fullscreen pipeline is loaded
    if (!EnsureDefaultPipeline(device)) {
        LOG_ERROR("CompositionPass", "Failed to create default pipeline");
        return false;
    }

    // Create composite descriptor set
    if (m_pipelineState) {
        auto layouts = m_pipelineState->GetDescriptorSetLayouts();
        if (!layouts.empty()) {
            m_compositeDescSetLayout = layouts[0];
            m_compositeDescSet = factory->CreateDescriptorSet(m_compositeDescSetLayout.get());
            if (m_compositeDescSet) {
                m_compositeDescSet->BindTexture(0, nullptr, nullptr);
                m_compositeDescSet->Update();
            }
        }
    }

    LOG_INFO("CompositionPass", "Setup complete");
    return true;
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
    if (m_postProcessSettings.fog) m_stats.postProcessEffects++;
}

void CompositionPass::Execute(ICommandBuffer* cmd, IRenderDevice* device) {
    if (!cmd || !device) return;

    // Determine render resolution from swap chain
    uint32_t w = 1, h = 1;
    auto* swapChain = device->GetSwapChain();
    if (swapChain) {
        w = static_cast<uint32_t>(swapChain->GetWidth());
        h = static_cast<uint32_t>(swapChain->GetHeight());
    }

    m_stats = {};
    m_stats.postProcessEffects = 0;

    // Determine which effects are active
    bool doSSR = m_postProcessSettings.ssr && m_ssrPass && m_ssrPass->IsReady();
    bool doFog = m_postProcessSettings.fog && m_fogPass && m_fogPass->IsReady();
    bool doTonemap = m_postProcessSettings.toneMapping && m_tonemapPass && m_tonemapPass->IsReady();

    if (doSSR) m_stats.postProcessEffects++;
    if (doFog) m_stats.postProcessEffects++;
    if (doTonemap) m_stats.postProcessEffects++;

    // --- Step 1: Screen-Space Reflections ---
    if (doSSR) {
        m_ssrPass->Execute(cmd, m_gbColor, m_gbNormal, m_gbDepth, m_gbPosition, m_outputTexture);
    }

    // --- Step 2: Fog ---
    if (doFog && m_gbDepth && m_gbPosition) {
        ITexture* sceneSrc = m_inputTexture;
        m_fogPass->Execute(cmd, sceneSrc, m_gbDepth, m_gbPosition, m_outputTexture);
    }

    // --- Step 3: Tonemapping (HDR -> LDR) ---
    ITexture* tonemapSource = m_outputTexture ? m_outputTexture : m_inputTexture;
    if (doTonemap) {
        m_tonemapPass->Execute(cmd, tonemapSource, m_outputTexture);
    }

    // --- Step 4: Final Composite (fullscreen triangle) ---
    if (!EnsureDefaultPipeline(device)) return;

    cmd->SetPipelineState(m_pipelineState.get());

    cmd->SetViewport(Viewport{0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h), 0.0f, 1.0f});
    cmd->SetScissorRect(Rect{0, 0, static_cast<int>(w), static_cast<int>(h)});

    // Bind output texture as input to the fragment shader
    ITexture* compositeSource = m_outputTexture ? m_outputTexture : m_inputTexture;
    if (compositeSource && m_compositeDescSet) {
        m_compositeDescSet->BindTexture(0, compositeSource, m_linearSampler.get());
        m_compositeDescSet->Update();

        // Bind descriptor set at set 0 for the default pipeline
        cmd->BindDescriptorSet(0, m_compositeDescSet.get());

        // Transition to shader read
        std::vector<ImageBarrier> barriers;
        barriers.push_back({compositeSource, ResourceState::UnorderedAccess, ResourceState::ShaderRead});
        cmd->PipelineBarrier(barriers);
    }

    // Push constants for the composite fragment shader
    struct PushData {
        float exposure;
        float gamma;
        int debugView;
        float padding;
    };
    PushData pd{};
    pd.exposure = m_toneMappingParams.exposure;
    pd.gamma = m_toneMappingParams.gamma;
    pd.debugView = 0;
    pd.padding = 0.0f;

    cmd->PushConstants(ShaderType::Vertex, &pd, sizeof(pd));
    cmd->PushConstants(ShaderType::Pixel, &pd, sizeof(pd));

    // Fullscreen triangle draw (3 vertices, no index buffer)
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
    case PostProcessEffect::Fog:
        m_postProcessSettings.fog = enable; break;
    case PostProcessEffect::None: break;
    }
}

bool CompositionPass::IsPostProcessEffectEnabled(PostProcessEffect effect) const {
    switch (effect) {
    case PostProcessEffect::ToneMapping:     return m_postProcessSettings.toneMapping;
    case PostProcessEffect::GammaCorrection: return m_postProcessSettings.gammaCorrection;
    case PostProcessEffect::FXAA:            return m_postProcessSettings.fxaa;
    case PostProcessEffect::SMAA:            return m_postProcessSettings.smaa;
    case PostProcessEffect::Bloom:           return m_postProcessSettings.bloom;
    case PostProcessEffect::SSR:             return m_postProcessSettings.ssr;
    case PostProcessEffect::SSAO:            return m_postProcessSettings.ssao;
    case PostProcessEffect::DepthOfField:   return m_postProcessSettings.depthOfField;
    case PostProcessEffect::Fog:            return m_postProcessSettings.fog;
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

    // Use copy passthrough shader for final blit (compute passes handle tone/SSR/fog)
    m_fragmentShader = resourceManager->LoadShaderSync(
        "assets/shaders/deferred_composite_copy.frag.spv", "main");
    if (!m_fragmentShader) {
        // Fallback to original composite shader
        m_fragmentShader = resourceManager->LoadShaderSync(
            "assets/shaders/deferred_composite.frag.spv", "main");
    }

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
