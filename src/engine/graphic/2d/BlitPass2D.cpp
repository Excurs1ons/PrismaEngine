#include "BlitPass2D.h"
#include "graphic/Renderer.h"
#include "graphic/Mesh.h"
#include "graphic/Material.h"
#include "graphic/RenderResourceManager.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/IPipelineState.h"
#include "graphic/interfaces/IBuffer.h"
#include "graphic/interfaces/ITexture.h"
#include "graphic/interfaces/IDescriptorSet.h"
#include "graphic/RenderSystem.h"
#include "Logger.h"
#include "app/Engine.h"

namespace Prisma::Graphic {

struct alignas(16) BlitPushConstants {
    PrismaMath::mat4 mvp;
};

// 与 UnlitSprite.frag 对齐: set=0 binding=0 (std140)
struct alignas(16) BlitMaterialData {
    PrismaMath::vec4 baseColor{1,1,1,1};
    float metallic = 0;
    float roughness = 0;
    float ao = 1;
    float emissiveIntensity = 0;
};

BlitPass2D::BlitPass2D()
    : Pass2D("BlitPass2D") {
}

BlitPass2D::~BlitPass2D() = default;

bool BlitPass2D::Initialize(IRenderDevice* device, IRenderResourceManager* rm, TextureFormat rtFormat) {
    if (!device || !rm) return false;

    m_vertShader = rm->LoadShaderSync("assets/shaders/Renderer2D.vert.spv", "main");
    m_fragShader = rm->LoadShaderSync("assets/shaders/UnlitSprite.frag.spv", "main");

    if (!m_vertShader) {
        m_vertShader = rm->LoadShaderSync("Default");
    }
    if (!m_fragShader) {
        m_fragShader = rm->LoadShaderSync("DefaultPixel");
    }
    if (!m_vertShader || !m_fragShader) {
        LOG_ERROR("BlitPass2D", "无法加载 2D 着色器");
        return false;
    }

    if (!createPSO(device, rm, rtFormat)) return false;
    if (!createDescriptorSet(device, rm)) return false;

    return true;
}

bool BlitPass2D::createPSO(IRenderDevice* device, IRenderResourceManager* rm, TextureFormat rtFormat) {
    auto* rf = device->GetResourceFactory();
    if (!rf) return false;

    PipelineStateDesc desc;
    desc.vertexShader = m_vertShader;
    desc.pixelShader  = m_fragShader;
    desc.primitiveTopology = PrimitiveTopology::TriangleList;

    // Alpha blending
    desc.blendState.blendEnable = true;
    desc.blendState.srcBlend    = BlendFactorType::SrcAlpha;
    desc.blendState.destBlend   = BlendFactorType::InvSrcAlpha;
    desc.blendState.blendOp     = BlendOp::Add;

    desc.depthStencilState.depthEnable = false;
    desc.rasterizerState.cullMode = CullMode::None;

    desc.renderTargetFormats[0] = rtFormat;
    desc.numRenderTargets = 1;

    // Vertex2D 输入布局: posUV(vec4) at 0, color(vec4) at 16
    desc.inputLayout = {
        { "POSUV", 0, TextureFormat::RGBA32_Float, 0, 0  },
        { "COLOR", 0, TextureFormat::RGBA32_Float, 0, 16 },
    };

    m_pso = rm->CreatePipelineState(desc);
    if (!m_pso) {
        LOG_ERROR("BlitPass2D", "创建 PSO 失败");
        return false;
    }
    return true;
}

bool BlitPass2D::createDescriptorSet(IRenderDevice* device, IRenderResourceManager* rm) {
    auto* rf = device->GetResourceFactory();
    if (!rf) return false;

    // 1. 材质 UBO (set=0, binding=0)
    BufferDesc uboDesc;
    uboDesc.type = BufferType::Constant;
    uboDesc.size = sizeof(BlitMaterialData);
    uboDesc.usage = BufferUsage::Dynamic;
    m_materialUBO = rf->CreateBufferImpl(uboDesc);

    // 2. Descriptor set layout: set=0, binding=0(UBO), binding=1(sampler2D)
    std::vector<ShaderResource> shaderResources;
    {
        ShaderResource uboRes;
        uboRes.Name = "MaterialData";
        uboRes.ResourceType = ShaderResource::Type::UniformBuffer;
        uboRes.Set = 0;
        uboRes.Binding = 0;
        shaderResources.push_back(uboRes);
    }
    {
        ShaderResource texRes;
        texRes.Name = "AlbedoMap";
        texRes.ResourceType = ShaderResource::Type::Sampler2D;
        texRes.Set = 0;
        texRes.Binding = 1;
        shaderResources.push_back(texRes);
    }

    m_dsLayout = rf->CreateDescriptorSetLayout(shaderResources);
    m_dsArray.resize(FRAME_OVERLAP);

    // 创建一块共享的默认白纹理，供所有帧的 descriptor set 共用
    {
        TextureDesc defaultTexDesc;
        defaultTexDesc.width = 1;
        defaultTexDesc.height = 1;
        defaultTexDesc.format = TextureFormat::RGBA8_UNorm;
        defaultTexDesc.allowShaderResource = true;
        m_defaultTexture = rf->CreateTextureImpl(defaultTexDesc);
        if (m_defaultTexture) {
            m_defaultTexture->Clear(Color(1, 1, 1, 1));
        }
    }
    auto defaultSampler = rm ? rm->GetDefaultSampler() : nullptr;

    for (uint32_t i = 0; i < FRAME_OVERLAP; ++i) {
        auto ds = rf->CreateDescriptorSet(m_dsLayout.get());
        if (!ds || !m_materialUBO) {
            LOG_ERROR("BlitPass2D", "创建 descriptor set [{}] 失败", i);
            return false;
        }

        // 绑定 UBO 到 binding 0
        ds->BindBuffer(0, m_materialUBO.get(), 0, sizeof(BlitMaterialData), DescriptorType::UniformBuffer);

        // 绑定共享默认纹理到 binding 1
        if (m_defaultTexture && defaultSampler) {
            ds->BindTexture(1, m_defaultTexture.get(), defaultSampler.get());
        }

        ds->Update();
        m_dsArray[i] = std::move(ds);
    }
    return true;
}

void BlitPass2D::Draw(ICommandBuffer* cmd, const std::vector<RenderCommand>& commands,
                      const PrismaMath::mat4& mvp) {
    if (!cmd || commands.empty() || !m_pso || m_dsArray.empty()) return;

    // 使用当前帧索引选择描述符集，避免 VUID 03047（更新正在被引用的描述符集）
    auto* device = Engine::Get().GetRenderSystem() ? Engine::Get().GetRenderSystem()->GetDevice() : nullptr;
    uint32_t frameIndex = device ? device->GetCurrentFrameIndex() % FRAME_OVERLAP : 0;
    auto ds = m_dsArray[frameIndex].get();

    cmd->SetPipelineState(m_pso.get());

    auto* rm = Engine::Get().GetRenderResourceManager();
    auto defaultSampler = rm ? rm->GetDefaultSampler() : nullptr;

    // 预先收集所有纹理绑定，在 BindDescriptorSet 之前完成更新
    Material* lastMaterial = nullptr;
    bool descriptorDirty = false;
    for (const auto& command : commands) {
        if (!command.mesh) continue;

        if (command.material && command.material != lastMaterial) {
            std::shared_ptr<ITexture> tex = nullptr;
            if (auto* texVal = command.material->GetParam("AlbedoMap")) {
                if (std::holds_alternative<std::shared_ptr<ITexture>>(*texVal)) {
                    tex = std::get<std::shared_ptr<ITexture>>(*texVal);
                }
            }
            if (tex && defaultSampler) {
                ds->BindTexture(1, tex.get(), defaultSampler.get());
                descriptorDirty = true;
            }
            lastMaterial = command.material;
        }
    }

    // 在绑定前一次性更新描述符集
    if (descriptorDirty) {
        ds->Update();
    }

    // 绑定描述符集（更新完成后）
    cmd->BindDescriptorSet(0, ds);

    // 第二遍：绘制所有命令
    lastMaterial = nullptr;
    for (const auto& command : commands) {
        if (!command.mesh) continue;

        // UBO 更新材质颜色
        if (command.material && command.material != lastMaterial) {
            BlitMaterialData matData;
            matData.baseColor = PrismaMath::vec4(command.color.r, command.color.g, command.color.b, command.color.a);
            if (auto* bc = command.material->GetParam("BaseColor")) {
                if (std::holds_alternative<PrismaMath::vec4>(*bc))
                    matData.baseColor = std::get<PrismaMath::vec4>(*bc);
            }
            m_materialUBO->UpdateData(&matData, sizeof(matData), 0);

            lastMaterial = command.material;
        }

        // Push 常量: MVP（颜色已通过 UBO 传递）
        BlitPushConstants pc{};
        pc.mvp = mvp * command.transform;
        cmd->PushConstants(ShaderType::VertexAndPixel, &pc, sizeof(pc));

        // 绘制每个子网格
        for (const auto& subMesh : command.mesh->GetSubMeshes()) {
            if (subMesh.vertexBuffer && subMesh.indexBuffer) {
                cmd->SetVertexBuffer(subMesh.vertexBuffer.get(), 0);
                cmd->SetIndexBuffer(subMesh.indexBuffer.get());
                cmd->DrawIndexed(subMesh.indexCount);
            }
        }
    }
}

} // namespace Prisma::Graphic
