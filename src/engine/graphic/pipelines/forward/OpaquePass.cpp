#include "OpaquePass.h"
#include "app/Engine.h"
#include "Platform.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IDescriptorSet.h"
#include "graphic/interfaces/IPipelineState.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/IResourceManager.h"
#include "graphic/Mesh.h"
#include "graphic/Material.h"
#include "graphic/Shader.h"
#include "graphic/interfaces/ISwapChain.h"
#include "logger/Logger.h"

// 离屏管线支持所需（Bug 5 修复）
#include "adapters/vulkan/VulkanPipelineState.h"
#include "adapters/vulkan/RenderDeviceVulkan.h"

#include <fstream>
#include <iterator>

namespace Prisma::Graphic {

OpaquePass::OpaquePass() : ForwardRenderPass("OpaquePass") {}

namespace {
struct alignas(16) QuadPushConstants {
    PrismaMath::mat4 mvp;
    Prisma::Color color;
};

// PBR push constants — matches lit.vert layout(push_constant)
struct alignas(16) PBRPushConstants {
    PrismaMath::mat4 world;
    Prisma::Color color;
};

// Scene UBO data — matches lit.frag SceneData (std140)
struct alignas(16) SceneData {
    PrismaMath::mat4 view;
    PrismaMath::mat4 projection;
    PrismaMath::mat4 viewProjection;
    PrismaMath::vec4 cameraPos; // xyz = position, w = padding
};

// Max PBR lights — matches pbr_lit.frag SSBO capacity
constexpr uint32_t kMaxPBRLights = 32;
}

void OpaquePass::SetLights(const std::vector<Light>& lights) {
    m_Lights = lights;
}

void OpaquePass::Update(Prisma::Timestep ts) {
    ForwardRenderPass::Update(ts);
}

void OpaquePass::Execute(const PassExecutionContext& context) {
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

void OpaquePass::Execute(ICommandBuffer* cmd, const std::vector<RenderCommand>& commands) {
    if (!cmd || commands.empty() || !m_device) {
        return;
    }

    // 检测材质类型以选择管线 — 扫描所有命令，若有任意 PBR 则使用 PBR 路径
    bool usePBR = false;
    for (const auto& c : commands) {
        if (c.material && c.material->GetMaterialType() == MaterialType::PBR) {
            usePBR = true;
            break;
        }
    }

    if (usePBR) {
        // ── PBR 渲染路径 ──
        if (!EnsurePBRPipeline()) {
            LOG_ERROR("OpaquePass", "确保 PBR 管线失败，跳过绘制");
            return;
        }
        EnsurePBRDescriptors();

        // 选择 PBR PSO
        if (m_useOffscreen) {
            if (!m_pbrOffscreenPipelineState) {
                LOG_ERROR("OpaquePass", "PBR 离屏 PSO 未创建，跳过绘制");
                return;
            }
            cmd->SetPipelineState(m_pbrOffscreenPipelineState.get());
        } else {
            cmd->SetPipelineState(m_pbrSwapchainPipelineState.get());
        }

        // 更新场景和光源数据
        UpdateSceneUBO();
        UpdateLightBuffer();

        // 绑定场景 Set 1、光源 Set 2、阴影 Set 3
        if (m_sceneDescriptorSet) {
            cmd->BindDescriptorSet(1, m_sceneDescriptorSet.get());
        }
        if (m_lightDescriptorSet) {
            cmd->BindDescriptorSet(2, m_lightDescriptorSet.get());
        }
        if (m_shadowDescriptorSet) {
            cmd->BindDescriptorSet(3, m_shadowDescriptorSet.get());
        }

        Material* lastMaterial = nullptr;
        for (const auto& command : commands) {
            if (!command.mesh) continue;

            if (command.material && command.material != lastMaterial) {
                command.material->Bind(cmd);
                lastMaterial = command.material;
            }

            PBRPushConstants pushConstants{};
            pushConstants.world = command.transform;
            pushConstants.color = command.color;
            cmd->PushConstants(ShaderType::Vertex, &pushConstants, sizeof(pushConstants));

            for (const auto& subMesh : command.mesh->GetSubMeshes()) {
                if (subMesh.vertexBuffer && subMesh.indexBuffer) {
                    cmd->SetVertexBuffer(subMesh.vertexBuffer.get(), 0);
                    cmd->SetIndexBuffer(subMesh.indexBuffer.get());
                    cmd->DrawIndexed(subMesh.indexCount);
                }
            }
        }
    } else {
        // ── 原有 Unlit/LitSprite 渲染路径 ──
        if (m_useOffscreen) {
            if (!EnsureOffscreenPipeline()) {
                LOG_ERROR("OpaquePass", "确保离屏管线失败，跳过绘制");
                return;
            }
            cmd->SetPipelineState(m_offscreenPipelineState.get());
        } else {
            if (!EnsureSwapchainPipeline()) {
                LOG_ERROR("OpaquePass", "确保交换链管线失败，跳过绘制");
                return;
            }
            cmd->SetPipelineState(m_swapchainPipelineState.get());
        }

        static double lastLog = 0;
        double now = Platform::GetTimeSeconds();
        if (now - lastLog >= 5.0) {
            LOG_DEBUG("OpaquePass", "绘制 {} 条命令, PSO={}, shaders ok={}",
                     commands.size(), (void*)(m_useOffscreen ? m_offscreenPipelineState.get() : m_swapchainPipelineState.get()),
                     m_defaultVertexShader && m_defaultPixelShader);
            lastLog = now;
        }

        Material* lastMaterial = nullptr;
        for (const auto& command : commands) {
            if (!command.mesh) continue;

            if (command.material && command.material != lastMaterial) {
                command.material->Bind(cmd);
                lastMaterial = command.material;
            }

            QuadPushConstants pushConstants{};
            pushConstants.mvp = m_projection * m_view * command.transform;
            pushConstants.color = command.color;
            cmd->PushConstants(ShaderType::VertexAndPixel, &pushConstants, sizeof(pushConstants));

            for (const auto& subMesh : command.mesh->GetSubMeshes()) {
                if (subMesh.vertexBuffer && subMesh.indexBuffer) {
                    cmd->SetVertexBuffer(subMesh.vertexBuffer.get(), 0);
                    cmd->SetIndexBuffer(subMesh.indexBuffer.get());
                    cmd->DrawIndexed(subMesh.indexCount);
                }
            }
        }
    }
}

bool OpaquePass::EnsureSwapchainPipeline() {
    if (m_swapchainPipelineState) {
        return true;
    }
    if (!m_device || !m_device->GetResourceFactory()) {
        return false;
    }

    auto resourceManager = Engine::Get().GetRenderResourceManager();
    if (!resourceManager) {
        return false;
    }

    // 使用 RenderResourceManager 加载着色器，这会自动处理搜索路径和内置反射
    m_defaultVertexShader = resourceManager->LoadShaderSync("assets/shaders/Renderer2D.vert.spv", "main");
    m_defaultPixelShader = resourceManager->LoadShaderSync("assets/shaders/UnlitSprite.frag.spv", "main");

    // 如果加载失败，尝试使用内置的 Default 着色器作为保底
    if (!m_defaultVertexShader) {
        LOG_WARNING("OpaquePass", "无法加载 Renderer2D 顶点着色器，尝试使用内置默认着色器。");
        m_defaultVertexShader = resourceManager->LoadShaderSync("Default");
    }
    
    if (!m_defaultPixelShader) {
        LOG_WARNING("OpaquePass", "无法加载 Renderer2D 片段着色器，尝试使用内置默认着色器。");
        m_defaultPixelShader = resourceManager->LoadShaderSync("DefaultPixel");
    }

    if (!m_defaultVertexShader || !m_defaultPixelShader) {
        LOG_ERROR("OpaquePass", "无法加载必要的着色器资源，OpaquePass 无法正常工作。");
        return false;
    }

    auto pso = m_device->GetResourceFactory()->CreatePipelineStateImpl();
    if (!pso) {
        return false;
    }

    pso->SetShader(ShaderType::Vertex, m_defaultVertexShader);
    pso->SetShader(ShaderType::Pixel, m_defaultPixelShader);
    pso->SetPrimitiveTopology(PrimitiveTopology::TriangleList);
    
    // 设置基础状态
    RasterizerState rs;
    rs.cullMode = CullMode::None; // 2D 渲染通常不开启裁剪
    pso->SetRasterizerState(rs);

    std::vector<VertexInputAttribute> attributes = {
        { "POSITION", 0, TextureFormat::RGB32_Float, 0, 0  },
        { "TEXCOORD", 0, TextureFormat::RG32_Float,  0, 12 },
        { "COLOR",    0, TextureFormat::RGBA32_Float, 0, 20 },
    };
    pso->SetInputLayout(attributes);

    if (!pso->Create(m_device)) {
        LOG_ERROR("OpaquePass", "创建 Renderer2D 管线失败: {0}", pso->GetErrors());
        return false;
    }

    m_swapchainPipelineState = std::shared_ptr<IPipelineState>(std::move(pso));
    return true;
}

void OpaquePass::CreatePipelineForRenderPass(VkRenderPass rp) {
    m_offscreenRenderPass = rp;
    LOG_DEBUG("OpaquePass", "离屏管线已配置 RenderPass: 0x{:x}", reinterpret_cast<uintptr_t>(rp));
}

bool OpaquePass::EnsureOffscreenPipeline() {
    if (m_offscreenPipelineState) {
        return true;
    }
    if (!m_device || !m_device->GetResourceFactory() || m_offscreenRenderPass == VK_NULL_HANDLE) {
        LOG_ERROR("OpaquePass", "离屏管线未配置 RenderPass 或设备无效");
        return false;
    }

    auto resourceManager = Engine::Get().GetRenderResourceManager();
    if (!resourceManager) {
        return false;
    }

    // 如果着色器尚未加载，先调用 EnsureSwapchainPipeline 加载共享着色器
    if (!m_defaultVertexShader || !m_defaultPixelShader) {
        if (!EnsureSwapchainPipeline()) {
            return false;
        }
    }

    if (!m_defaultVertexShader || !m_defaultPixelShader) {
        LOG_ERROR("OpaquePass", "无法加载着色器资源，离屏管线无法创建。");
        return false;
    }

    auto pso = m_device->GetResourceFactory()->CreatePipelineStateImpl();
    if (!pso) {
        return false;
    }

    pso->SetShader(ShaderType::Vertex, m_defaultVertexShader);
    pso->SetShader(ShaderType::Pixel, m_defaultPixelShader);
    pso->SetPrimitiveTopology(PrimitiveTopology::TriangleList);

    RasterizerState rs;
    rs.cullMode = CullMode::None;
    pso->SetRasterizerState(rs);

    std::vector<VertexInputAttribute> attributes = {
        { "POSITION", 0, TextureFormat::RGB32_Float, 0, 0  },
        { "TEXCOORD", 0, TextureFormat::RG32_Float,  0, 12 },
        { "COLOR",    0, TextureFormat::RGBA32_Float, 0, 20 },
    };
    pso->SetInputLayout(attributes);

    auto* vkPSO = dynamic_cast<Vulkan::VulkanPipelineState*>(pso.get());
    if (!vkPSO) {
        LOG_ERROR("OpaquePass", "创建的 PSO 不是 VulkanPipelineState");
        return false;
    }
    vkPSO->SetCustomRenderPass(m_offscreenRenderPass);

    if (!pso->Create(m_device)) {
        LOG_ERROR("OpaquePass", "创建离屏管线失败: {0}", pso->GetErrors());
        return false;
    }

    m_offscreenPipelineState = std::shared_ptr<IPipelineState>(std::move(pso));
    LOG_DEBUG("OpaquePass", "离屏 PSO 创建成功");
    return true;
}

bool OpaquePass::EnsurePBRPipeline() {
    if (m_pbrSwapchainPipelineState) {
        return true;
    }

    auto* factory = m_device->GetResourceFactory();
    if (!factory) {
        LOG_ERROR("OpaquePass", "EnsurePBRPipeline: Resource factory is null");
        return false;
    }

    auto resourceManager = Engine::Get().GetRenderResourceManager();
    if (!resourceManager) {
        LOG_ERROR("OpaquePass", "EnsurePBRPipeline: Resource manager is null");
        return false;
    }

    m_pbrVertexShader = resourceManager->LoadShaderSync("assets/shaders/pbr_lit.vert.spv", "main");
    if (!m_pbrVertexShader) {
        LOG_ERROR("OpaquePass", "无法加载 PBR 顶点着色器: assets/shaders/pbr_lit.vert.spv");
        return false;
    }

    m_pbrPixelShader = resourceManager->LoadShaderSync("assets/shaders/pbr_lit.frag.spv", "main");
    if (!m_pbrPixelShader) {
        LOG_ERROR("OpaquePass", "无法加载 PBR 片段着色器: assets/shaders/pbr_lit.frag.spv");
        return false;
    }

    auto createPBRPSO = [&](VkRenderPass customRP) -> std::shared_ptr<IPipelineState> {
        auto pso = factory->CreatePipelineStateImpl();
        if (!pso) return nullptr;

        pso->SetShader(ShaderType::Vertex, m_pbrVertexShader);
        pso->SetShader(ShaderType::Pixel, m_pbrPixelShader);
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
        rs.frontCounterClockwise = true;
        pso->SetRasterizerState(rs);

        DepthStencilState ds{};
        ds.depthEnable = true;
        ds.depthWriteEnable = true;
        ds.depthFunc = ComparisonFunc::Less;
        pso->SetDepthStencilState(ds);

        if (customRP) {
            auto* vkPSO = dynamic_cast<Vulkan::VulkanPipelineState*>(pso.get());
            if (vkPSO) {
                vkPSO->SetCustomRenderPass(customRP);
            }
        }

        if (!pso->Create(m_device)) {
            LOG_ERROR("OpaquePass", "PBR PSO 创建失败: {0}", pso->GetErrors());
            return nullptr;
        }

        return std::shared_ptr<IPipelineState>(std::move(pso));
    };

    m_pbrSwapchainPipelineState = createPBRPSO(nullptr);
    if (!m_pbrSwapchainPipelineState) {
        return false;
    }

    if (m_offscreenRenderPass != nullptr && !m_pbrOffscreenPipelineState) {
        m_pbrOffscreenPipelineState = createPBRPSO(m_offscreenRenderPass);
        if (!m_pbrOffscreenPipelineState) {
            LOG_WARNING("OpaquePass", "PBR 离屏 PSO 创建失败，将使用交换链 PSO 回落");
        }
    }

    LOG_DEBUG("OpaquePass", "PBR 管线创建成功 (vert={}, frag={})",
              static_cast<void*>(m_pbrVertexShader.get()),
              static_cast<void*>(m_pbrPixelShader.get()));
    return true;
}

void OpaquePass::EnsurePBRDescriptors() {
    if (m_sceneDescriptorSet) return;

    auto* factory = m_device->GetResourceFactory();
    if (!factory) {
        LOG_ERROR("OpaquePass", "EnsurePBRDescriptors: Resource factory is null");
        return;
    }

    uint32_t lightBufferSize = sizeof(Light) * kMaxPBRLights;

    // Scene UBO (Set 1, Binding 0)
    BufferDesc uboDesc;
    uboDesc.type = BufferType::Constant;
    uboDesc.size  = sizeof(Prisma::Graphic::SceneData);
    uboDesc.usage = BufferUsage::Dynamic;
    m_sceneUBO = factory->CreateBufferImpl(uboDesc);
    if (!m_sceneUBO) {
        LOG_ERROR("OpaquePass", "创建场景 UBO 失败");
        return;
    }

    // Light SSBO (Set 2, Binding 0)
    BufferDesc ssboDesc;
    ssboDesc.type = BufferType::Structured;
    ssboDesc.size = lightBufferSize;
    ssboDesc.usage = BufferUsage::Dynamic;
    m_lightBuffer = factory->CreateBufferImpl(ssboDesc);
    if (!m_lightBuffer) {
        LOG_ERROR("OpaquePass", "创建光源 SSBO 失败");
        return;
    }

    // Scene descriptor set (Set 1)
    {
        std::vector<ShaderResource> resources;
        ShaderResource res;
        res.Name = "SceneData";
        res.ResourceType = ShaderResource::Type::UniformBuffer;
        res.Set = 1;
        res.Binding = 0;
        resources.push_back(res);

        m_sceneDescriptorSetLayout = factory->CreateDescriptorSetLayout(resources);
        m_sceneDescriptorSet = factory->CreateDescriptorSet(m_sceneDescriptorSetLayout.get());
        if (m_sceneDescriptorSet) {
            m_sceneDescriptorSet->BindBuffer(
                0, m_sceneUBO.get(), 0, sizeof(Prisma::Graphic::SceneData), DescriptorType::UniformBuffer);
            m_sceneDescriptorSet->Update();
        }
    }

    // Light descriptor set (Set 2)
    {
        std::vector<ShaderResource> resources;
        ShaderResource res;
        res.Name = "LightBuffer";
        res.ResourceType = ShaderResource::Type::StorageBuffer;
        res.Set = 2;
        res.Binding = 0;
        resources.push_back(res);

        m_lightDescriptorSetLayout = factory->CreateDescriptorSetLayout(resources);
        m_lightDescriptorSet = factory->CreateDescriptorSet(m_lightDescriptorSetLayout.get());
        if (m_lightDescriptorSet) {
            m_lightDescriptorSet->BindBuffer(0, m_lightBuffer.get(), 0, lightBufferSize, DescriptorType::StorageBuffer);
            m_lightDescriptorSet->Update();
        }
    }

    // Shadow map descriptor set (Set 3)
    // Binding 0: shadow cascade textures as a combined image sampler array
    {
        std::vector<ShaderResource> resources;
        ShaderResource res;
        res.Name = "ShadowMapArray";
        res.ResourceType = ShaderResource::Type::Sampler2D;
        res.Set = 3;
        res.Binding = 0;
        resources.push_back(res);

        m_shadowDescriptorSetLayout = factory->CreateDescriptorSetLayout(resources);
        m_shadowDescriptorSet = factory->CreateDescriptorSet(m_shadowDescriptorSetLayout.get());
        if (m_shadowDescriptorSet) {
            // 临时绑定一个空描述符；实际阴影纹理由 ForwardPipeline 在每帧绑定
            m_shadowDescriptorSet->Update();
        }
    }
}

void OpaquePass::UpdateSceneUBO() {
    if (!m_sceneUBO) return;

    Prisma::Graphic::SceneData data{};
    data.camera.view = m_view;
    data.camera.projection = m_projection;
    data.camera.viewProjection = m_viewProjection;
    data.camera.position = PrismaMath::vec4(m_cameraPos.x, m_cameraPos.y, m_cameraPos.z, 0.0f);

    m_sceneUBO->UpdateData(&data, sizeof(data), 0);
}

void OpaquePass::UpdateLightBuffer() {
    if (!m_lightBuffer) return;

    // 填充光源数据到栈上数组，最多 kMaxPBRLights 个
    Light lights[kMaxPBRLights] = {};
    uint32_t count = std::min(static_cast<uint32_t>(m_Lights.size()), kMaxPBRLights);
    for (uint32_t i = 0; i < count; ++i) {
        lights[i] = m_Lights[i];
    }
    // 剩余槽位保持零值（无效光源，着色器中 length(radiance) < EPSILON 会跳过）

    m_lightBuffer->UpdateData(lights, sizeof(lights), 0);
}

} // namespace Prisma::Graphic
