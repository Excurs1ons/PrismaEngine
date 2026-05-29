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

    // 根据当前模式选择交换链 PSO 或离屏 PSO
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
    // 视口/裁剪由调用方（Pipeline2D / ForwardPipeline）在 RenderPass begin 时设置，
    // 此处不再覆盖，避免破坏离屏 RT（如 PixelPerfect 的 256×224）的视口。

    // 缓存上一次绑定的材质指针，跳过重复绑定
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
    m_defaultPixelShader = resourceManager->LoadShaderSync("assets/shaders/LitSprite.frag.spv", "main");

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

} // namespace Prisma::Graphic
