#include "NPROpaquePass.h"
#include "app/Engine.h"
#include "Platform.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IDescriptorSet.h"
#include "graphic/interfaces/IPipelineState.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/IResourceManager.h"
#include "graphic/interfaces/IBuffer.h"
#include "graphic/Mesh.h"
#include "graphic/Material.h"
#include "graphic/Shader.h"
#include "graphic/interfaces/ISwapChain.h"
#include "logger/Logger.h"

namespace Prisma::Graphic {

// NPRMaterialData std140: 96 bytes
// 匹配 GLSL 声明在 npr_lit.frag 的 Set 0, Binding 0
namespace {
struct alignas(16) NPRSceneUBO {
    PrismaMath::mat4 view;
    PrismaMath::mat4 projection;
    PrismaMath::mat4 viewProjection;
    PrismaMath::vec4 cameraPos;
};

struct alignas(16) NPRPushConstants {
    PrismaMath::mat4 world;
    PrismaMath::vec4 color;
};
} // anonymous namespace

NPROpaquePass::NPROpaquePass() : ForwardRenderPass("NPROpaquePass") {}

void NPROpaquePass::SetLights(const std::vector<Light>& lights) {
    m_Lights = lights;
}

void NPROpaquePass::Update(Prisma::Timestep ts) {
    ForwardRenderPass::Update(ts);
}

void NPROpaquePass::Execute(const PassExecutionContext& context) {
    if (!context.deviceContext) {
        return;
    }

    if (context.renderTarget && context.depthStencil) {
        context.deviceContext->SetRenderTarget(context.renderTarget, context.depthStencil);
    } else if (context.renderTarget) {
        context.deviceContext->SetRenderTarget(context.renderTarget);
    }

    if (context.sceneData) {
        context.deviceContext->SetViewport(
            0.0f,
            0.0f,
            static_cast<float>(context.sceneData->viewport.width),
            static_cast<float>(context.sceneData->viewport.height)
        );
    }

    context.deviceContext->GpuMemoryBarrier();
}

void NPROpaquePass::Execute(ICommandBuffer* cmd, const std::vector<RenderCommand>& commands) {
    if (!cmd || commands.empty() || !m_device) {
        return;
    }
    if (!EnsureDefaultPipeline()) {
        LOG_ERROR("NPROpaquePass", "Unable to create default pipeline, skipping draw");
        return;
    }

    // ── Set up viewport ──
    cmd->SetPipelineState(m_defaultPipelineState.get());
    float width = 1.0f;
    float height = 1.0f;
    if (auto* swapChain = m_device->GetSwapChain()) {
        width = static_cast<float>(swapChain->GetWidth());
        height = static_cast<float>(swapChain->GetHeight());
    }
    cmd->SetViewport(Viewport{0.0f, 0.0f, width, height, 0.0f, 1.0f});
    cmd->SetScissorRect(Rect{0, 0, static_cast<int>(width), static_cast<int>(height)});

    // ── Update Set 1: SceneData ──
    NPRSceneUBO sceneUbo{};
    sceneUbo.view = m_view;
    sceneUbo.projection = m_projection;
    sceneUbo.viewProjection = m_viewProjection;
    sceneUbo.cameraPos = PrismaMath::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    if (m_sceneUBO) {
        m_sceneUBO->UpdateData(&sceneUbo, sizeof(sceneUbo), 0);
    }
    if (m_sceneDS) {
        cmd->BindDescriptorSet(1, m_sceneDS.get());
    }

    // ── Update Set 3: Lights ──
    if (!m_Lights.empty() && m_lightBufferGPU && m_lightCountUBO) {
        m_lightBufferGPU->UpdateData(m_Lights.data(), m_Lights.size() * sizeof(Light), 0);
        uint32_t numLights = static_cast<uint32_t>(m_Lights.size());
        m_lightCountUBO->UpdateData(&numLights, sizeof(numLights), 0);
    }
    if (m_lightsDS) {
        cmd->BindDescriptorSet(3, m_lightsDS.get());
    }

    // ── Draw loop ──
    for (const auto& command : commands) {
        if (!command.mesh) continue;

        // ── Update Set 0: NPR Material Data ──
        Material::NPRMaterialData nprData{};
        if (command.material) {
            if (auto* val = command.material->GetParam("BaseColor")) {
                if (std::holds_alternative<PrismaMath::vec4>(*val))
                    nprData.baseColor = std::get<PrismaMath::vec4>(*val);
            }
            if (auto* val = command.material->GetParam("RimColor")) {
                if (std::holds_alternative<PrismaMath::vec4>(*val))
                    nprData.rimColor = std::get<PrismaMath::vec4>(*val);
            }
            if (auto* val = command.material->GetParam("Roughness")) {
                if (std::holds_alternative<float>(*val))
                    nprData.roughness = std::get<float>(*val);
            }
            if (auto* val = command.material->GetParam("EmissiveIntensity")) {
                if (std::holds_alternative<float>(*val))
                    nprData.emissiveIntensity = std::get<float>(*val);
            }
            if (auto* val = command.material->GetParam("RimPower")) {
                if (std::holds_alternative<float>(*val))
                    nprData.rimPower = std::get<float>(*val);
            }
            if (auto* val = command.material->GetParam("WrapAmount")) {
                if (std::holds_alternative<float>(*val))
                    nprData.wrapAmount = std::get<float>(*val);
            }
        }
        if (m_materialUBO) {
            m_materialUBO->UpdateData(&nprData, sizeof(nprData), 0);
        }

        if (m_materialDS) {
            cmd->BindDescriptorSet(0, m_materialDS.get());
        }

        // ── Push constants (world matrix + color) ──
        NPRPushConstants pushConstants{};
        pushConstants.world = command.transform;
        pushConstants.color = PrismaMath::vec4(command.color.r, command.color.g, command.color.b, command.color.a);
        cmd->PushConstants(ShaderType::Vertex, &pushConstants, sizeof(pushConstants));
        cmd->PushConstants(ShaderType::Pixel, &pushConstants, sizeof(pushConstants));

        for (const auto& subMesh : command.mesh->GetSubMeshes()) {
            if (subMesh.vertexBuffer && subMesh.indexBuffer) {
                cmd->SetVertexBuffer(subMesh.vertexBuffer.get(), 0);
                cmd->SetIndexBuffer(subMesh.indexBuffer.get());
                cmd->DrawIndexed(subMesh.indexCount);
            }
        }
    }
}

bool NPROpaquePass::EnsureDefaultPipeline() {
    if (m_defaultPipelineState) {
        return true;
    }
    if (!m_device || !m_device->GetResourceFactory()) {
        return false;
    }

    auto resourceManager = Engine::Get().GetRenderResourceManager();
    if (!resourceManager) {
        return false;
    }

    // ── Load NPR shaders ──
    m_defaultVertexShader = resourceManager->LoadShaderSync("assets/shaders/npr_lit.vert.spv", "main");
    m_defaultPixelShader = resourceManager->LoadShaderSync("assets/shaders/npr_lit.frag.spv", "main");

    if (!m_defaultVertexShader) {
        LOG_WARNING("NPROpaquePass", "Cannot load npr_lit.vert.spv, trying default shader");
        m_defaultVertexShader = resourceManager->LoadShaderSync("Default");
    }
    if (!m_defaultPixelShader) {
        LOG_WARNING("NPROpaquePass", "Cannot load npr_lit.frag.spv, trying default shader");
        m_defaultPixelShader = resourceManager->LoadShaderSync("DefaultPixel");
    }

    if (!m_defaultVertexShader || !m_defaultPixelShader) {
        LOG_ERROR("NPROpaquePass", "Failed to load NPR shaders");
        return false;
    }

    auto pso = m_device->GetResourceFactory()->CreatePipelineStateImpl();
    if (!pso) {
        return false;
    }

    pso->SetShader(ShaderType::Vertex, m_defaultVertexShader);
    pso->SetShader(ShaderType::Pixel, m_defaultPixelShader);
    pso->SetPrimitiveTopology(PrimitiveTopology::TriangleList);

    std::vector<VertexInputAttribute> attributes = {
        { "POSITION",  0, TextureFormat::RGBA32_Float, 0, 0   },
        { "COLOR",     0, TextureFormat::RGBA32_Float, 0, 16  },
        { "TEXCOORD",  0, TextureFormat::RGBA32_Float, 0, 32  },
        { "NORMAL",    0, TextureFormat::RGBA32_Float, 0, 48  },
        { "TEXCOORD2", 0, TextureFormat::RGBA32_Float, 0, 64  },
        { "TANGENT",   0, TextureFormat::RGBA32_Float, 0, 80  },
    };
    pso->SetInputLayout(attributes);

    RasterizerState rs;
    rs.cullMode = CullMode::Back;
    pso->SetRasterizerState(rs);

    DepthStencilState ds{};
    ds.depthEnable = true;
    ds.depthWriteEnable = true;
    ds.depthFunc = ComparisonFunc::LessEqual;
    pso->SetDepthStencilState(ds);

    if (!pso->Create(m_device)) {
        LOG_ERROR("NPROpaquePass", "Failed to create PSO: {0}", pso->GetErrors());
        return false;
    }

    m_defaultPipelineState = std::shared_ptr<IPipelineState>(std::move(pso));

    // ── Create descriptor sets from PSO's auto-generated layouts ──
    const auto& layouts = m_defaultPipelineState->GetDescriptorSetLayouts();

    auto* factory = m_device->GetResourceFactory();
    auto* rm = resourceManager;
    auto defaultSampler = rm->GetDefaultSampler();

    // Set 0: NPR Material Data (0=NPRMaterialData UBO, 1=albedo, 2=normal, 3=metallicRoughness)
    if (layouts.size() >= 1) {
        m_materialDSLayout = layouts[0];

        BufferDesc uboDesc;
        uboDesc.type = BufferType::Constant;
        uboDesc.size = sizeof(Material::NPRMaterialData);
        uboDesc.usage = BufferUsage::Dynamic;
        m_materialUBO = factory->CreateBufferImpl(uboDesc);

        m_materialDS = factory->CreateDescriptorSet(m_materialDSLayout.get());
        m_materialDS->BindBuffer(0, m_materialUBO.get(), 0, sizeof(Material::NPRMaterialData), DescriptorType::UniformBuffer);

        uint32_t whitePixel = 0xFFFFFFFF;
        TextureDesc whiteDesc;
        whiteDesc.width = 1;
        whiteDesc.height = 1;
        whiteDesc.format = TextureFormat::RGBA8_UNorm;
        auto defaultWhite = rm->CreateTextureFromMemory(&whitePixel, sizeof(whitePixel), whiteDesc);
        m_materialDS->BindTexture(1, defaultWhite.get(), defaultSampler.get());
        m_materialDS->BindTexture(2, defaultWhite.get(), defaultSampler.get());
        m_materialDS->BindTexture(3, defaultWhite.get(), defaultSampler.get());

        m_materialDS->Update();
    }

    // Set 1: SceneData (camera UBO)
    if (layouts.size() >= 2) {
        m_sceneDSLayout = layouts[1];

        BufferDesc sceneDesc;
        sceneDesc.type = BufferType::Constant;
        sceneDesc.size = sizeof(NPRSceneUBO);
        sceneDesc.usage = BufferUsage::Dynamic;
        m_sceneUBO = factory->CreateBufferImpl(sceneDesc);

        m_sceneDS = factory->CreateDescriptorSet(m_sceneDSLayout.get());
        m_sceneDS->BindBuffer(0, m_sceneUBO.get(), 0, sizeof(NPRSceneUBO), DescriptorType::UniformBuffer);
        m_sceneDS->Update();
    }

    // Set 3: Lights (SSBO + light count UBO)
    if (layouts.size() >= 4) {
        m_lightsDSLayout = layouts[3];

        BufferDesc lightBufDesc;
        lightBufDesc.type = BufferType::Structured;
        lightBufDesc.size = 1024;
        lightBufDesc.usage = BufferUsage::Dynamic;
        m_lightBufferGPU = factory->CreateBufferImpl(lightBufDesc);

        BufferDesc countDesc;
        countDesc.type = BufferType::Constant;
        countDesc.size = sizeof(uint32_t);
        countDesc.usage = BufferUsage::Dynamic;
        m_lightCountUBO = factory->CreateBufferImpl(countDesc);

        m_lightsDS = factory->CreateDescriptorSet(m_lightsDSLayout.get());
        m_lightsDS->BindBuffer(0, m_lightBufferGPU.get(), 0, m_lightBufferGPU->GetSize(), DescriptorType::StorageBuffer);
        m_lightsDS->BindBuffer(1, m_lightCountUBO.get(), 0, sizeof(uint32_t), DescriptorType::UniformBuffer);
        m_lightsDS->Update();
    }

    return true;
}

} // namespace Prisma::Graphic
