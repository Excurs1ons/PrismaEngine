#include "LightingPass.h"
#include "app/Engine.h"
#include "graphic/interfaces/IResourceManager.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/ISwapChain.h"
#include "graphic/RenderResourceManager.h"
#include "Logger.h"

namespace Prisma::Graphic {

LightingPass::LightingPass()
    : LogicalPass("LightingPass")
    , m_gBuffer(nullptr)
    , m_ambientLight(0.1f, 0.1f, 0.1f)
    , m_iblEnabled(true)
    , m_irradianceMap(nullptr)
    , m_prefilterMap(nullptr)
    , m_brdfLUT(nullptr)
{
    m_priority = 150;
}

void LightingPass::Update(Timestep ts) {
    UpdateTime(ts);
}

void LightingPass::Execute(const PassExecutionContext& context) {
    if (!context.deviceContext) return;
    m_stats = {};

    context.deviceContext->SetViewport(0.0f, 0.0f,
        static_cast<float>(context.sceneData->viewport.width),
        static_cast<float>(context.sceneData->viewport.height));

    float ambientData[4] = {m_ambientLight.x, m_ambientLight.y, m_ambientLight.z, 1.0f};
    context.deviceContext->SetConstantData(0, ambientData, sizeof(ambientData));

    if (m_gBuffer) {
        m_gBuffer->BindAsShaderResources(context.deviceContext, 0);
    }

    m_stats.lightsRendered = static_cast<uint32_t>(m_lights.size());
    m_stats.shadowCastingLights = 0;
    for (const auto& light : m_lights) {
        if (light.castShadows) m_stats.shadowCastingLights++;
    }

    if (m_iblEnabled && m_irradianceMap && m_prefilterMap && m_brdfLUT) {
    }
}

void LightingPass::Execute(ICommandBuffer* cmd, IRenderDevice* device) {
    if (!cmd || !device) return;
    if (!EnsureDefaultPipeline(device)) return;

    m_stats = {};
    m_stats.lightsRendered = static_cast<uint32_t>(m_lights.size());

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
        float ambient[4];
        float lightDir[4];
        float lightColor[4];
        float lightPos[4];
    };
    PushData pd{};
    pd.ambient[0] = m_ambientLight.x;
    pd.ambient[1] = m_ambientLight.y;
    pd.ambient[2] = m_ambientLight.z;
    pd.ambient[3] = 1.0f;
    if (!m_lights.empty()) {
        pd.lightDir[0] = m_lights[0].direction.x;
        pd.lightDir[1] = m_lights[0].direction.y;
        pd.lightDir[2] = m_lights[0].direction.z;
        pd.lightDir[3] = 1.0f;
        pd.lightColor[0] = m_lights[0].color.x * m_lights[0].intensity;
        pd.lightColor[1] = m_lights[0].color.y * m_lights[0].intensity;
        pd.lightColor[2] = m_lights[0].color.z * m_lights[0].intensity;
        pd.lightColor[3] = 1.0f;
        pd.lightPos[0] = m_lights[0].position.x;
        pd.lightPos[1] = m_lights[0].position.y;
        pd.lightPos[2] = m_lights[0].position.z;
        pd.lightPos[3] = m_lights[0].range;
    } else {
        pd.lightDir[2] = -1.0f;
        pd.lightDir[3] = 1.0f;
        pd.lightColor[3] = 1.0f;
        pd.lightPos[3] = 1.0f;
    }

    cmd->PushConstants(ShaderType::Vertex, &pd, sizeof(pd));
    cmd->PushConstants(ShaderType::Pixel, &pd, sizeof(pd));

    cmd->Draw(3, 1);
}

void LightingPass::SetIBLTextures(ITexture* irradianceMap, ITexture* prefilterMap, ITexture* brdfLUT) {
    m_irradianceMap = irradianceMap;
    m_prefilterMap = prefilterMap;
    m_brdfLUT = brdfLUT;
}

bool LightingPass::EnsureDefaultPipeline(IRenderDevice* device) {
    if (m_pipelineState) return true;
    if (!device || !device->GetResourceFactory()) return false;

    auto* resourceManager = Engine::Get().GetRenderResourceManager();
    if (!resourceManager) return false;

    m_vertexShader = resourceManager->LoadShaderSync(
        "assets/shaders/deferred_fullscreen.vert.spv", "main");
    m_fragmentShader = resourceManager->LoadShaderSync(
        "assets/shaders/deferred_lighting.frag.spv", "main");

    if (!m_vertexShader || !m_fragmentShader) {
        LOG_ERROR("LightingPass", "shader loading failed");
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
        LOG_ERROR("LightingPass", "PSO creation failed: {}", pso->GetErrors());
        return false;
    }
    m_pipelineState = std::shared_ptr<IPipelineState>(std::move(pso));
    LOG_INFO("LightingPass", "PSO created");
    return true;
}

} // namespace Prisma::Graphic
