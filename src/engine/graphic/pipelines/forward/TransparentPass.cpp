#include "TransparentPass.h"
#include "app/Engine.h"
#include "graphic/interfaces/ICommandBuffer.h"
#include "graphic/interfaces/IRenderDevice.h"
#include "graphic/interfaces/IPipelineState.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "graphic/interfaces/IResourceManager.h"
#include "graphic/Mesh.h"
#include "graphic/Material.h"
#include "graphic/RenderSystem.h"
#include "adapters/vulkan/VulkanPipelineState.h"
#include "adapters/vulkan/RenderDeviceVulkan.h"
#include "logger/Logger.h"

namespace Prisma::Graphic {

TransparentPass::TransparentPass()
    : ForwardRenderPass("TransparentPass")
    , m_depthWrite(false)
    , m_depthTest(true)
{
    m_priority = 300;
}

bool TransparentPass::Setup(IRenderDevice* device) {
    if (!device) {
        LOG_ERROR("TransparentPass", "Setup 收到空设备指针");
        return false;
    }

    m_device = device;

    auto resourceManager = Engine::Get().GetRenderResourceManager();
    if (!resourceManager) {
        LOG_ERROR("TransparentPass", "Setup 无法获取资源管理器");
        return false;
    }

    // 预加载着色器资源（与 EnsureSwapchainPipeline 共享着色器缓存）
    m_defaultVertexShader = resourceManager->LoadShaderSync(
        "assets/shaders/Renderer2D.vert.spv", "main");
    if (!m_defaultVertexShader) {
        LOG_WARNING("TransparentPass", "Setup 无法加载 Renderer2D 顶点着色器，尝试使用内置默认着色器。");
        m_defaultVertexShader = resourceManager->LoadShaderSync("Default");
    }

    m_defaultPixelShader = resourceManager->LoadShaderSync(
        "assets/shaders/UnlitVertex.frag.spv", "main");
    if (!m_defaultPixelShader) {
        LOG_WARNING("TransparentPass", "Setup 无法加载 UnlitVertex 片段着色器，尝试使用内置默认像素着色器。");
        m_defaultPixelShader = resourceManager->LoadShaderSync("DefaultPixel");
    }

    if (!m_defaultVertexShader || !m_defaultPixelShader) {
        LOG_ERROR("TransparentPass", "Setup 无法加载必要的着色器资源");
        return false;
    }

    LOG_DEBUG("TransparentPass", "Setup 完成（着色器预加载成功）");
    return true;
}

void TransparentPass::Update(Timestep ts) {
    UpdateTime(ts);
}

void TransparentPass::Execute(const PassExecutionContext& context) {
    if (!context.deviceContext) {
        return;
    }

    m_stats = {};

    if (!m_device) {
        auto* renderSystem = Engine::Get().GetRenderSystem();
        if (renderSystem) {
            m_device = renderSystem->GetDevice();
        }
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

    if (!EnsureSwapchainPipeline()) {
        LOG_ERROR("TransparentPass", "无法创建透明管线 PSO，跳过绘制");
        return;
    }

    context.deviceContext->SetPipelineState(m_swapchainPipelineState.get());

    const auto& commands = Renderer::GetCommandQueue();

    Material* lastMaterial = nullptr;
    for (const auto& cmd : commands) {
        if (!cmd.mesh) continue;

        // IDeviceContext 不支持 Material::Bind（需要 ICommandBuffer），
        // 完整材质绑定在 ICommandBuffer Execute 版本中处理
        if (cmd.material && cmd.material != lastMaterial) {
            lastMaterial = cmd.material;
        }

        const PrismaMath::mat4 mvp = m_viewProjection * cmd.transform;

        context.deviceContext->SetConstantData(0, &mvp, sizeof(PrismaMath::mat4));
        context.deviceContext->SetConstantData(1, &cmd.color, sizeof(Prisma::Color));

        for (const auto& subMesh : cmd.mesh->GetSubMeshes()) {
            if (!subMesh.vertexBuffer || !subMesh.indexBuffer) continue;

            context.deviceContext->SetVertexBuffer(
                subMesh.vertexBuffer.get(),
                0,
                0,
                kVertexStride
            );
            context.deviceContext->SetIndexBuffer(
                subMesh.indexBuffer.get(),
                0,
                !subMesh.use16BitIndices
            );
            context.deviceContext->DrawIndexed(
                subMesh.indexCount,
                subMesh.baseIndex,
                static_cast<int32_t>(subMesh.baseVertex)
            );

            ++m_stats.drawCalls;
            m_stats.triangles += subMesh.indexCount / 3;
        }
        ++m_stats.transparentObjects;
    }
}

void TransparentPass::Execute(ICommandBuffer* cmd, const std::vector<RenderCommand>& commands) {
    if (!cmd || commands.empty()) {
        return;
    }

    if (!m_device) {
        auto* renderSystem = Engine::Get().GetRenderSystem();
        if (renderSystem) {
            m_device = renderSystem->GetDevice();
        }
    }

    if (m_useOffscreen) {
        if (!EnsureOffscreenPipeline()) {
            LOG_ERROR("TransparentPass", "确保离屏管线失败，跳过绘制");
            return;
        }
        cmd->SetPipelineState(m_offscreenPipelineState.get());
    } else {
        if (!EnsureSwapchainPipeline()) {
            LOG_ERROR("TransparentPass", "确保交换链管线失败，跳过绘制");
            return;
        }
        cmd->SetPipelineState(m_swapchainPipelineState.get());
    }

    m_stats = {};

    Material* lastMaterial = nullptr;
    for (const auto& command : commands) {
        if (!command.mesh) continue;

        if (command.material && command.material != lastMaterial) {
            command.material->Bind(cmd);
            lastMaterial = command.material;
        }

        TransparentPushConstants pushConstants{};
        pushConstants.mvp = m_projection * m_view * command.transform;
        pushConstants.color = command.color;
        cmd->PushConstants(ShaderType::Vertex, &pushConstants, sizeof(pushConstants));
        cmd->PushConstants(ShaderType::Pixel, &pushConstants, sizeof(pushConstants));

        for (const auto& subMesh : command.mesh->GetSubMeshes()) {
            if (subMesh.vertexBuffer && subMesh.indexBuffer) {
                cmd->SetVertexBuffer(subMesh.vertexBuffer.get(), 0);
                cmd->SetIndexBuffer(subMesh.indexBuffer.get(), !subMesh.use16BitIndices);
                cmd->DrawIndexed(subMesh.indexCount);

                ++m_stats.drawCalls;
                m_stats.triangles += subMesh.indexCount / 3;
            }
        }
        ++m_stats.transparentObjects;
    }
}

bool TransparentPass::EnsureSwapchainPipeline() {
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

    m_defaultVertexShader = resourceManager->LoadShaderSync(
        "assets/shaders/Renderer2D.vert.spv", "main");
    m_defaultPixelShader = resourceManager->LoadShaderSync(
        "assets/shaders/UnlitVertex.frag.spv", "main");

    if (!m_defaultVertexShader) {
        LOG_WARNING("TransparentPass", "无法加载 Renderer2D 顶点着色器，尝试默认着色器。");
        m_defaultVertexShader = resourceManager->LoadShaderSync("Default");
    }
    if (!m_defaultPixelShader) {
        LOG_WARNING("TransparentPass", "无法加载 UnlitVertex 片段着色器，尝试默认像素着色器。");
        m_defaultPixelShader = resourceManager->LoadShaderSync("DefaultPixel");
    }

    if (!m_defaultVertexShader || !m_defaultPixelShader) {
        LOG_ERROR("TransparentPass", "无法加载必要的着色器资源。");
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

    DepthStencilState ds;
    ds.depthEnable = m_depthTest;
    ds.depthWriteEnable = m_depthWrite;
    ds.depthFunc = ComparisonFunc::Less;
    pso->SetDepthStencilState(ds);

    // SrcAlpha + OneMinusSrcAlpha 标准透明混合
    BlendState bs;
    bs.blendEnable = true;
    bs.srcBlend = BlendFactorType::SrcAlpha;
    bs.destBlend = BlendFactorType::InvSrcAlpha;
    bs.blendOp = BlendOp::Add;
    bs.srcBlendAlpha = BlendFactorType::One;
    bs.destBlendAlpha = BlendFactorType::Zero;
    bs.blendOpAlpha = BlendOp::Add;
    bs.writeMask = 0xF;
    pso->SetBlendState(bs);

    std::vector<VertexInputAttribute> attributes = {
        { "POSITION", 0, TextureFormat::RGB32_Float, 0, 0  },
        { "TEXCOORD", 0, TextureFormat::RG32_Float,  0, 12 },
        { "COLOR",    0, TextureFormat::RGBA32_Float, 0, 20 },
    };
    pso->SetInputLayout(attributes);

    if (!pso->Create(m_device)) {
        LOG_ERROR("TransparentPass", "创建交换链 PSO 失败: {}", pso->GetErrors());
        return false;
    }

    m_swapchainPipelineState = std::shared_ptr<IPipelineState>(std::move(pso));
    LOG_DEBUG("TransparentPass", "交换链 PSO 创建成功 (SrcAlpha+InvSrcAlpha 混合)");
    return true;
}

void TransparentPass::CreatePipelineForRenderPass(VkRenderPass rp) {
    m_offscreenRenderPass = rp;
    LOG_DEBUG("TransparentPass", "离屏管线已配置 RenderPass: 0x{:x}",
              reinterpret_cast<uintptr_t>(rp));
}

bool TransparentPass::EnsureOffscreenPipeline() {
    if (m_offscreenPipelineState) {
        return true;
    }
    if (!m_device || !m_device->GetResourceFactory() || m_offscreenRenderPass == VK_NULL_HANDLE) {
        LOG_ERROR("TransparentPass", "离屏管线未配置 RenderPass 或设备无效");
        return false;
    }

    auto resourceManager = Engine::Get().GetRenderResourceManager();
    if (!resourceManager) {
        return false;
    }

    if (!m_defaultVertexShader || !m_defaultPixelShader) {
        if (!EnsureSwapchainPipeline()) {
            return false;
        }
    }

    if (!m_defaultVertexShader || !m_defaultPixelShader) {
        LOG_ERROR("TransparentPass", "无法加载着色器资源，离屏管线无法创建。");
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

    DepthStencilState ds;
    ds.depthEnable = m_depthTest;
    ds.depthWriteEnable = m_depthWrite;
    ds.depthFunc = ComparisonFunc::Less;
    pso->SetDepthStencilState(ds);

    BlendState bs;
    bs.blendEnable = true;
    bs.srcBlend = BlendFactorType::SrcAlpha;
    bs.destBlend = BlendFactorType::InvSrcAlpha;
    bs.blendOp = BlendOp::Add;
    bs.srcBlendAlpha = BlendFactorType::One;
    bs.destBlendAlpha = BlendFactorType::Zero;
    bs.blendOpAlpha = BlendOp::Add;
    bs.writeMask = 0xF;
    pso->SetBlendState(bs);

    std::vector<VertexInputAttribute> attributes = {
        { "POSITION", 0, TextureFormat::RGB32_Float, 0, 0  },
        { "TEXCOORD", 0, TextureFormat::RG32_Float,  0, 12 },
        { "COLOR",    0, TextureFormat::RGBA32_Float, 0, 20 },
    };
    pso->SetInputLayout(attributes);

    // 转换到 VulkanPipelineState 以设置自定义 RenderPass
    auto* vkPSO = dynamic_cast<Vulkan::VulkanPipelineState*>(pso.get());
    if (!vkPSO) {
        LOG_ERROR("TransparentPass", "创建的 PSO 不是 VulkanPipelineState（离屏需要 Vulkan）");
        return false;
    }
    vkPSO->SetCustomRenderPass(m_offscreenRenderPass);

    if (!pso->Create(m_device)) {
        LOG_ERROR("TransparentPass", "创建离屏 PSO 失败: {}", pso->GetErrors());
        return false;
    }

    m_offscreenPipelineState = std::shared_ptr<IPipelineState>(std::move(pso));
    LOG_DEBUG("TransparentPass", "离屏 PSO 创建成功");
    return true;
}

void TransparentPass::Cleanup() {
    m_swapchainPipelineState.reset();
    m_offscreenPipelineState.reset();
    m_defaultVertexShader.reset();
    m_defaultPixelShader.reset();
    m_psoCache.clear();
    m_device = nullptr;
    m_offscreenRenderPass = nullptr;
    m_useOffscreen = false;
    m_stats = {};
}

} // namespace Prisma::Graphic
