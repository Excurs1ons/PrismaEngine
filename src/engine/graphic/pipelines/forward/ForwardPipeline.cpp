#include "ForwardPipeline.h"
#include "DepthPrePass.h"
#include "OpaquePass.h"
#include "TransparentPass.h"
#include "../../2d/Light2DPass.h"
#include "../../2d/ReflectionPass2D.h"
#include "../SkyboxRenderPass.h"
#include "graphic/Renderer.h"
#include "graphic/Renderer2D.h"
#include "graphic/RenderCommandContext.h"
#include "graphic/interfaces/IResourceManager.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "app/Engine.h"
#include "Logger.h"

// Vulkan 特定代码支持
#include "adapters/vulkan/RenderDeviceVulkan.h"
#include "adapters/vulkan/VulkanResources.h"
#include "graphic/interfaces/IRenderTarget.h"

namespace Prisma::Graphic {

namespace {
struct alignas(16) GizmoPushConstants {
    PrismaMath::mat4 mvp;
    Prisma::Color color;
};
}

/**
 * @brief 内部渲染目标代理
 * 用于将 ITexture 包装为 IRenderTarget，以便传递给 Pass。
 */
class TextureRenderTargetProxy final : public ITextureRenderTarget {
public:
    TextureRenderTargetProxy(ITexture* texture) : m_texture(texture) {}

    uint32_t GetWidth() const override { return m_texture ? static_cast<uint32_t>(m_texture->GetWidth()) : 0; }
    uint32_t GetHeight() const override { return m_texture ? static_cast<uint32_t>(m_texture->GetHeight()) : 0; }
    TextureFormat GetFormat() const override { return m_texture ? m_texture->GetFormat() : TextureFormat::Unknown; }
    TextureType GetType() const override { return m_texture ? m_texture->GetTextureType() : TextureType::Texture2D; }
    
    void* GetNativeHandle() const override {
        if (!m_texture) return nullptr;
        auto vkTexture = dynamic_cast<Vulkan::VulkanTexture*>(m_texture);
        if (vkTexture) {
            return reinterpret_cast<void*>(vkTexture->GetVkImageView());
        }
        return nullptr;
    }

    bool IsSwapChain() const override { return false; }
    void Clear(const float color[4]) override {
        if (m_texture) {
            m_texture->Clear(Color(color[0], color[1], color[2], color[3]));
        }
    }

    uint32_t GetMipLevels() const override { return m_texture ? m_texture->GetMipLevels() : 0; }
    uint32_t GetArraySize() const override { return m_texture ? m_texture->GetArraySize() : 0; }
    ITexture* GetTexture() override { return m_texture; }

private:
    ITexture* m_texture;
};

ForwardPipeline::ForwardPipeline() = default;

ForwardPipeline::~ForwardPipeline() {
    Shutdown();
}

int ForwardPipeline::Initialize(IRenderDevice* device) {
    m_device = device;
    m_depthPrePass = std::make_shared<DepthPrePass>();
    m_opaquePass = std::make_shared<OpaquePass>();
    m_opaquePass->SetDevice(device);
    m_light2DPass = std::make_shared<Light2DPass>();
    m_reflectionPass2D = std::make_shared<ReflectionPass2D>();
    m_skyboxPass = std::make_shared<SkyboxPass>();
    m_transparentPass = std::make_shared<TransparentPass>();
    return 0;
}

void ForwardPipeline::Shutdown() {
    m_depthPrePass.reset();
    m_opaquePass.reset();
    m_light2DPass.reset();
    m_skyboxPass.reset();
    m_transparentPass.reset();
    m_reflectionPass2D.reset();
    m_gizmoPSO.reset();
    m_gizmoVertShader.reset();
    m_gizmoFragShader.reset();
}

void ForwardPipeline::EnsureGizmoPSO() {
    if (m_gizmoPSO) return;
    auto rm = Engine::Get().GetRenderResourceManager();
    if (!rm || !m_device) return;

    m_gizmoVertShader = rm->LoadShaderSync("assets/shaders/Renderer2D.vert.spv");
    m_gizmoFragShader = rm->LoadShaderSync("assets/shaders/UnlitVertex.frag.spv");
    if (!m_gizmoVertShader || !m_gizmoFragShader) return;

    auto pso = m_device->GetResourceFactory()->CreatePipelineStateImpl();
    if (!pso) return;
    pso->SetShader(ShaderType::Vertex, m_gizmoVertShader);
    pso->SetShader(ShaderType::Pixel, m_gizmoFragShader);
    pso->SetPrimitiveTopology(PrimitiveTopology::TriangleList);
    RasterizerState rs; rs.cullMode = CullMode::None;
    pso->SetRasterizerState(rs);
    if (pso->Create(m_device)) {
        m_gizmoPSO = std::shared_ptr<IPipelineState>(std::move(pso));
    }
}

void ForwardPipeline::Execute(const RenderContext& ctx) {
    if (!m_device) return;

    // -----------------------------------------------------------------------
    // [修复] 处理目标重定向
    // -----------------------------------------------------------------------
    if (ctx.targetTexture) {
        auto vulkanDevice = dynamic_cast<Vulkan::RenderDeviceVulkan*>(ctx.device);
        if (vulkanDevice) {
            // 确保不开启默认交换链 Pass
            vulkanDevice->SetSkipSwapChainRenderPass(true);
        }
    }

    TextureRenderTargetProxy proxy(ctx.targetTexture);

    const auto& commands = Renderer::GetCommandQueue();
    auto view = ctx.camera.viewMatrix;
    auto proj = ctx.camera.projectionMatrix;
    const PrismaMath::mat4 viewProjection = proj * view;

    RenderCommandContext fallbackContext;
    IDeviceContext* deviceContext = &fallbackContext;

    SceneData sceneData;
    sceneData.camera.view = view;
    sceneData.camera.projection = proj;
    sceneData.camera.viewProjection = viewProjection;
    sceneData.camera.position = ctx.camera.position;
    sceneData.camera.nearPlane = ctx.camera.nearPlane;
    sceneData.camera.farPlane = ctx.camera.farPlane;
    sceneData.time.ts = ctx.deltaTime;
    sceneData.viewport.width = ctx.width;
    sceneData.viewport.height = ctx.height;

    PassExecutionContext passContext;
    passContext.deviceContext = deviceContext;
    passContext.sceneData = &sceneData;
    passContext.renderTarget = ctx.targetTexture ? &proxy : nullptr;

    // TODO: 目前各 Pass 内部仍硬编码了对交换链 RenderPass 的依赖。
    // 在后续重构中，需要将 targetTexture 传入 Pass 内部。
    // 暂时保持逻辑链路畅通，修复嵌套崩溃。

    if (m_depthPrePass) {
        m_depthPrePass->SetViewMatrix(view);
        m_depthPrePass->SetProjectionMatrix(proj);
        m_depthPrePass->Execute(passContext);
    }

    // ⚡ Light2DPass 先于 OpaquePass 执行，确保光照纹理在场景渲染前准备就绪
    // 渲染结果会通过 SetLightTexture 传递给下一帧的 Renderer2D（一帧延迟可接受）
    //
    // [修复] Vulkan 禁止嵌套 RenderPass。BeginFrame 已开启交换链 RP，
    //        Light2DPass 需要自己的离屏 RP，因此必须先暂停交换链 RP。
    auto* vkDev = dynamic_cast<Vulkan::RenderDeviceVulkan*>(ctx.device);
    if (vkDev && !ctx.targetTexture) {
        vkDev->SuspendDefaultRenderPass();
    }

    if (m_light2DPass) {
        m_light2DPass->SetViewMatrix(view);
        m_light2DPass->SetProjectionMatrix(proj);
        m_light2DPass->Execute(passContext);
        if (ctx.commandBuffer) {
            m_light2DPass->ExecuteLight(ctx.commandBuffer, m_device, ctx.width, ctx.height);
        }
        // 将光照纹理传递给 Renderer2D，下一帧 OpaquePass 采样合成
        Renderer2D::SetLightTexture(m_light2DPass->GetLightTexture());
    }

    // Light2DPass 完成后，重新开启交换链 RP 供后续 Pass 使用
    if (vkDev && !ctx.targetTexture) {
        vkDev->ResumeDefaultRenderPass();
    }

    if (m_opaquePass) {
        m_opaquePass->SetViewMatrix(view);
        m_opaquePass->SetProjectionMatrix(proj);
        m_opaquePass->SetLights(ctx.lights);
        m_opaquePass->Execute(passContext);
        if (ctx.commandBuffer) {
            // [修复] ICommandBuffer* 现在直接传递
            m_opaquePass->Execute(ctx.commandBuffer, commands);
        }
    }

    // ── ReflectionPass2D ──
    // 在 OpaquePass 绘制场景后，使用前一帧捕获的场景颜色
    // 对反射材质表面采样镜像 UV 坐标，实现水面倒影/镜面效果
    if (m_reflectionPass2D) {
        m_reflectionPass2D->SetViewMatrix(view);
        m_reflectionPass2D->SetProjectionMatrix(proj);
        m_reflectionPass2D->Execute(passContext);
        if (ctx.commandBuffer) {
            m_reflectionPass2D->ExecuteReflection(ctx.commandBuffer, m_device, ctx.width, ctx.height);
        }
    }

    if (m_skyboxPass) {
        m_skyboxPass->SetViewMatrix(view);
        m_skyboxPass->SetProjectionMatrix(proj);
        m_skyboxPass->Execute(passContext);
    }

    if (m_transparentPass) {
        m_transparentPass->SetViewMatrix(view);
        m_transparentPass->SetProjectionMatrix(proj);
        m_transparentPass->Execute(passContext);
    }

    // ── Gizmo Overlay Pass ──
    // 在场景渲染完成后绘制 Gizmo（网格、坐标轴、HUD），
    // 使用 UnlitVertex 着色器（纯顶点色，无纹理无光照）。
    if (ctx.commandBuffer) {
        const auto& gizmoCommands = Renderer::GetGizmoQueue();
        if (!gizmoCommands.empty()) {
            EnsureGizmoPSO();
            if (m_gizmoPSO) {
                ctx.commandBuffer->SetPipelineState(m_gizmoPSO.get());
                float w = ctx.width > 0 ? (float)ctx.width : 1.0f;
                float h = ctx.height > 0 ? (float)ctx.height : 1.0f;
                ctx.commandBuffer->SetViewport(Viewport{0.0f, 0.0f, w, h, 0.0f, 1.0f});
                ctx.commandBuffer->SetScissorRect(Rect{0, 0, (int)w, (int)h});

                for (const auto& cmd : gizmoCommands) {
                    if (!cmd.mesh) continue;
                    GizmoPushConstants pc{};
                    pc.mvp = proj * view * cmd.transform;
                    pc.color = cmd.color;
                    ctx.commandBuffer->PushConstants(ShaderType::Vertex, &pc, sizeof(pc));
                    ctx.commandBuffer->PushConstants(ShaderType::Pixel, &pc, sizeof(pc));

                    for (const auto& subMesh : cmd.mesh->GetSubMeshes()) {
                        if (subMesh.vertexBuffer && subMesh.indexBuffer) {
                            ctx.commandBuffer->SetVertexBuffer(subMesh.vertexBuffer.get(), 0);
                            ctx.commandBuffer->SetIndexBuffer(subMesh.indexBuffer.get());
                            ctx.commandBuffer->DrawIndexed(subMesh.indexCount);
                        }
                    }
                }
            }
            Renderer::ClearGizmoQueue();
        }
    }

    // ── Capture scene for next frame's reflections ──
    // 必须在所有渲染完成后执行，且不在任何 RenderPass 内部
    if (m_reflectionPass2D && ctx.commandBuffer) {
        m_reflectionPass2D->CaptureScene(ctx.commandBuffer, ctx.device, ctx.width, ctx.height);
    }
}

} // namespace Prisma::Graphic
