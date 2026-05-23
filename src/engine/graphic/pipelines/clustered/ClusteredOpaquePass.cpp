#include "ClusteredOpaquePass.h"
#include "app/Engine.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/IResourceManager.h"
#include "graphic/interfaces/IDescriptorSet.h"
#include "graphic/interfaces/IPipelineState.h"
#include "graphic/interfaces/IBuffer.h"
#include "graphic/Mesh.h"
#include "graphic/Material.h"
#include "graphic/RenderDesc.h"
#include "Logger.h"

namespace Prisma::Graphic {

struct ClusteredPushConstants {
    PrismaMath::mat4 world;
    PrismaMath::vec4 color;
};

struct ClusteredSceneUBO {
    PrismaMath::mat4 view;
    PrismaMath::mat4 projection;
    PrismaMath::mat4 viewProjection;
};

ClusteredOpaquePass::ClusteredOpaquePass() : ForwardRenderPass("ClusteredOpaquePass") {}

void ClusteredOpaquePass::Update(Prisma::Timestep ts) {
    ForwardRenderPass::Update(ts);
}

void ClusteredOpaquePass::Execute(const PassExecutionContext& /*context*/) {
}

void ClusteredOpaquePass::SetClusteredResources(IBuffer* lightBuffer, IBuffer* indexList, IBuffer* grid, IBuffer* cameraUBO) {
    m_lightBuffer = lightBuffer;
    m_globalLightIndexList = indexList;
    m_clusterGrid = grid;
    m_cameraUBO = cameraUBO;
}

void ClusteredOpaquePass::Execute(ICommandBuffer* cmd, const std::vector<RenderCommand>& commands, IRenderDevice* device) {
    if (!cmd || commands.empty() || !device) {
        return;
    }

    // [修复] 添加周期性日志控制
    static auto lastLogTime = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    bool shouldLog = std::chrono::duration_cast<std::chrono::seconds>(now - lastLogTime).count() >= 5;
    if (shouldLog) lastLogTime = now;

    if (!EnsurePipeline(device)) return;

    cmd->SetPipelineState(m_pso.get());
    
    // 更新 Scene UBO
    ClusteredSceneUBO sceneUbo{};
    sceneUbo.view = m_view;
    sceneUbo.projection = m_projection;
    sceneUbo.viewProjection = m_viewProjection;
    m_sceneUBO->UpdateData(&sceneUbo, sizeof(sceneUbo), 0);

    // 绑定 Set 1 (Scene)
    if (m_sceneDS) {
        cmd->BindDescriptorSet(1, m_sceneDS.get());
    }

    // 绑定 Set 2 (Clustered Data)
    if (m_clusteredDS) {
        cmd->BindDescriptorSet(2, m_clusteredDS.get());
    }

    uint32_t drawCount = 0;
    // 绘制
    for (size_t i = 0; i < commands.size(); ++i) {
        const auto& command = commands[i];
        if (!command.mesh) continue;

        // 绑定材质 (Set 0)
        if (command.material) {
            command.material->Bind(cmd);
        } else {
            // 兜底绑定，确保 Set 0 不为空
            auto* rm = Engine::Get().GetRenderResourceManager();
            // Renderer2D::Initialize 已经确保了 DefaultMaterial 存在
            // 这里简单处理
        }

        ClusteredPushConstants pc{};
        pc.world = command.transform;
        pc.color = PrismaMath::vec4(command.color.r, command.color.g, command.color.b, command.color.a);
        
        if (shouldLog) {
            LOG_INFO("ClusteredOpaquePass", "  Draw Command {}: Scale=[{}, {}, {}]", i, 
                glm::length(pc.world[0]), glm::length(pc.world[1]), glm::length(pc.world[2]));
        }

        cmd->PushConstants(ShaderType::Vertex, &pc, sizeof(pc));
        cmd->PushConstants(ShaderType::Pixel, &pc, sizeof(pc));

        for (const auto& subMesh : command.mesh->GetSubMeshes()) {
            if (subMesh.vertexBuffer && subMesh.indexBuffer) {
                cmd->SetVertexBuffer(subMesh.vertexBuffer.get(), 0);
                cmd->SetIndexBuffer(subMesh.indexBuffer.get());
                cmd->DrawIndexed(subMesh.indexCount);
                drawCount++;
            }
        }
    }
}

bool ClusteredOpaquePass::EnsurePipeline(IRenderDevice* device) {
    if (m_pso) return true;

    auto* rm = Engine::Get().GetRenderResourceManager();
    auto* factory = device->GetResourceFactory();

    m_vertexShader = rm->LoadShaderSync("assets/shaders/clustered/clustered_forward.vert.spv");
    m_pixelShader = rm->LoadShaderSync("assets/shaders/clustered/clustered_forward.frag.spv");

    if (!m_vertexShader || !m_pixelShader) return false;

    auto pso = factory->CreatePipelineStateImpl();
    pso->SetShader(ShaderType::Vertex, m_vertexShader);
    pso->SetShader(ShaderType::Pixel, m_pixelShader);
    
    // 顶点布局 (对应 Prisma::Vertex)
    std::vector<VertexInputAttribute> attributes = {
        { "POSITION", 0, TextureFormat::RGBA32_Float, 0, 0 },
        { "COLOR",    0, TextureFormat::RGBA32_Float, 0, 16 },
        { "TEXCOORD", 0, TextureFormat::RGBA32_Float, 0, 32 },
        { "NORMAL",   0, TextureFormat::RGBA32_Float, 0, 48 }
    };
    pso->SetInputLayout(attributes);

    RasterizerState rs{};
    rs.cullMode = CullMode::None; // 康奈尔盒子内部需要看到双面或内面
    pso->SetRasterizerState(rs);

    DepthStencilState ds{};
    ds.depthEnable = true;
    ds.depthWriteEnable = true;
    ds.depthFunc = ComparisonFunc::LessEqual;
    pso->SetDepthStencilState(ds);

    if (!pso->Create(device)) return false;
    m_pso = std::shared_ptr<IPipelineState>(std::move(pso));

    // 从 PSO 获取自动生成的布局
    const auto& layouts = m_pso->GetDescriptorSetLayouts();

    // Set 1: Scene
    if (layouts.size() >= 2) {
        BufferDesc sceneDesc;
        sceneDesc.type = BufferType::Constant;
        sceneDesc.size = sizeof(ClusteredSceneUBO);
        sceneDesc.usage = BufferUsage::Dynamic;
        m_sceneUBO = factory->CreateBufferImpl(sceneDesc);

        m_sceneDS = factory->CreateDescriptorSet(layouts[1].get());
        m_sceneDS->BindBuffer(0, m_sceneUBO.get(), 0, sizeof(ClusteredSceneUBO), DescriptorType::UniformBuffer);
        m_sceneDS->Update();
    }

    // Set 2: Clustered Data
    if (layouts.size() >= 3) {
        m_clusteredDS = factory->CreateDescriptorSet(layouts[2].get());
        if (m_lightBuffer) m_clusteredDS->BindBuffer(0, m_lightBuffer, 0, m_lightBuffer->GetSize(), DescriptorType::StorageBuffer);
        if (m_globalLightIndexList) m_clusteredDS->BindBuffer(1, m_globalLightIndexList, 0, m_globalLightIndexList->GetSize(), DescriptorType::StorageBuffer);
        if (m_clusterGrid) m_clusteredDS->BindBuffer(2, m_clusterGrid, 0, m_clusterGrid->GetSize(), DescriptorType::StorageBuffer);
        if (m_cameraUBO) m_clusteredDS->BindBuffer(3, m_cameraUBO, 0, m_cameraUBO->GetSize(), DescriptorType::UniformBuffer);
        m_clusteredDS->Update();
    }

    return true;
}

} // namespace Prisma::Graphic
