#include "GeometryPass.h"
#include "app/Engine.h"
#include "graphic/interfaces/IResourceManager.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/ISwapChain.h"
#include "graphic/RenderResourceManager.h"
#include "Logger.h"

namespace Prisma::Graphic {

GeometryPass::GeometryPass()
    : ForwardRenderPass("GeometryPass")
    , m_gBuffer(nullptr)
    , m_depthPrePass(true)
{
    m_priority = 50;
}

void GeometryPass::Update(Timestep ts) {
    UpdateTime(ts);
}

void GeometryPass::Execute(const PassExecutionContext& context) {
    if (!context.deviceContext) return;
    m_stats = {};

    context.deviceContext->SetViewport(0.0f, 0.0f,
        static_cast<float>(context.sceneData->viewport.width),
        static_cast<float>(context.sceneData->viewport.height));

    context.deviceContext->SetConstantData(0, &m_viewProjection, sizeof(PrismaMath::mat4));

    if (m_gBuffer) {
        m_gBuffer->SetAsRenderTarget(context.deviceContext);
    }
}

void GeometryPass::Execute(ICommandBuffer* cmd, const std::vector<RenderCommand>& commands, IRenderDevice* device) {
    if (!cmd || commands.empty() || !device) return;
    if (!EnsureDefaultPipeline(device)) return;

    m_stats = {};
    m_stats.objects = static_cast<uint32_t>(commands.size());

    cmd->SetPipelineState(m_defaultPipelineState.get());

    float w = 1.0f, h = 1.0f;
    auto* swapChain = device->GetSwapChain();
    if (swapChain) {
        w = static_cast<float>(swapChain->GetWidth());
        h = static_cast<float>(swapChain->GetHeight());
    }
    cmd->SetViewport(Viewport{0.0f, 0.0f, w, h, 0.0f, 1.0f});
    cmd->SetScissorRect(Rect{0, 0, static_cast<int>(w), static_cast<int>(h)});

    for (const auto& command : commands) {
        if (!command.mesh) continue;

        PrismaMath::mat4 mvp = m_projection * m_view * command.transform;

        struct PushConstants {
            PrismaMath::mat4 mvp;
            Prisma::Color color;
        };
        PushConstants pc{};
        pc.mvp = mvp;
        pc.color = command.color;

        cmd->PushConstants(ShaderType::Vertex, &pc, sizeof(pc));
        cmd->PushConstants(ShaderType::Pixel, &pc, sizeof(pc));

        m_stats.drawCalls++;
        for (const auto& subMesh : command.mesh->GetSubMeshes()) {
            if (subMesh.vertexBuffer && subMesh.indexBuffer) {
                cmd->SetVertexBuffer(subMesh.vertexBuffer.get(), 0);
                cmd->SetIndexBuffer(subMesh.indexBuffer.get());
                cmd->DrawIndexed(subMesh.indexCount);
                m_stats.triangles += subMesh.indexCount / 3;
            }
        }
    }
}

bool GeometryPass::EnsureDefaultPipeline(IRenderDevice* device) {
    if (m_defaultPipelineState) return true;
    if (!device || !device->GetResourceFactory()) return false;

    auto* resourceManager = Engine::Get().GetRenderResourceManager();
    if (!resourceManager) return false;

    m_defaultVertexShader = resourceManager->LoadShaderSync(
        "assets/shaders/Renderer2D.vert.spv", "main");
    m_defaultPixelShader = resourceManager->LoadShaderSync(
        "assets/shaders/LitSprite.frag.spv", "main");

    if (!m_defaultVertexShader)
        m_defaultVertexShader = resourceManager->LoadShaderSync("Default");
    if (!m_defaultPixelShader)
        m_defaultPixelShader = resourceManager->LoadShaderSync("DefaultPixel");

    if (!m_defaultVertexShader || !m_defaultPixelShader) {
        LOG_ERROR("GeometryPass", "shader loading failed");
        return false;
    }

    auto pso = device->GetResourceFactory()->CreatePipelineStateImpl();
    pso->SetShader(ShaderType::Vertex, m_defaultVertexShader);
    pso->SetShader(ShaderType::Pixel, m_defaultPixelShader);
    pso->SetPrimitiveTopology(PrimitiveTopology::TriangleList);

    RasterizerState rs;
    rs.cullMode = CullMode::Back;
    pso->SetRasterizerState(rs);

    pso->SetDepthStencilState(DepthStencilState::Default);

    BlendState bs;
    bs.blendEnable = false;
    pso->SetBlendState(bs);

    if (!pso->Create(device)) {
        LOG_ERROR("GeometryPass", "PSO creation failed: {}", pso->GetErrors());
        return false;
    }
    m_defaultPipelineState = std::shared_ptr<IPipelineState>(std::move(pso));
    LOG_INFO("GeometryPass", "PSO created");
    return true;
}

} // namespace Prisma::Graphic
