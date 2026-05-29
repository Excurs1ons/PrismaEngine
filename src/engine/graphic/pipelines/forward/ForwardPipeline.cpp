#include "ForwardPipeline.h"
#include "DepthPrePass.h"
#include "OpaquePass.h"
#include "TransparentPass.h"
#include "../../2d/PostProcessPass2D.h"
#include "../../2d/UIPass2D.h"
#include "../SkyboxRenderPass.h"
#include "graphic/Renderer.h"
#include "graphic/Renderer2D.h"
#include "graphic/RenderCommandContext.h"
#include "graphic/interfaces/IResourceManager.h"
#include "graphic/interfaces/IResourceFactory.h"
#include "app/Engine.h"
#include "Logger.h"

#include "graphic/interfaces/IRenderTarget.h"
#include "graphic/adapters/vulkan/VulkanResources.h"

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
    m_postProcessPass = std::make_shared<PostProcessPass2D>();
    m_uiPass = std::make_shared<UIPass2D>();
    m_skyboxPass = std::make_shared<SkyboxPass>();
    m_transparentPass = std::make_shared<TransparentPass>();
    return 0;
}

void ForwardPipeline::Shutdown() {
    m_depthPrePass.reset();
    m_opaquePass.reset();
    m_skyboxPass.reset();
    m_transparentPass.reset();
    m_postProcessPass.reset();
    m_uiPass.reset();
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
    DepthStencilState ds{};
    ds.depthEnable = false;
    ds.depthWriteEnable = false;
    pso->SetDepthStencilState(ds);
    if (pso->Create(m_device)) {
        m_gizmoPSO = std::shared_ptr<IPipelineState>(std::move(pso));
    }
}

void ForwardPipeline::Execute(const RenderContext& ctx) {
    if (!m_device) return;

    if (!ctx.targetTexture) {
        ctx.device->BeginSwapChainRenderPass(ctx.clearColor);
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

    if (m_opaquePass) {
        m_opaquePass->SetViewMatrix(view);
        m_opaquePass->SetProjectionMatrix(proj);
        m_opaquePass->SetLights(ctx.lights);
        if (ctx.commandBuffer) {
            // [修复] ICommandBuffer* 现在直接传递
            m_opaquePass->Execute(ctx.commandBuffer, commands);
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

    // ── Post Processing 2D ──
    // [规划中] 在所有 2D 和场景渲染完成后应用后处理
    if (m_postProcessPass && ctx.commandBuffer) {
        // m_postProcessPass->Process(ctx.commandBuffer, ctx.device, sceneResultTexture);
    }

    // ── UI Pass 2D ──
    // [规划中] UI 应该在最后渲染，且不受后处理影响
    if (m_uiPass && ctx.commandBuffer) {
        m_uiPass->RenderUI(ctx.commandBuffer, ctx.device, ctx.width, ctx.height);
    }

    if (!ctx.targetTexture) {
        ctx.device->EndSwapChainRenderPass();
    }
}

} // namespace Prisma::Graphic
