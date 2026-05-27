#include "NPRPipeline.h"
#include "NPROpaquePass.h"
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
struct alignas(16) PushConstants {
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

NPRPipeline::NPRPipeline() = default;

NPRPipeline::~NPRPipeline() {
    Shutdown();
}

int NPRPipeline::Initialize(IRenderDevice* device) {
    m_device = device;
    m_nprOpaquePass = std::make_shared<NPROpaquePass>();
    m_nprOpaquePass->SetDevice(device);
    m_skyboxPass = std::make_shared<SkyboxPass>();
    m_postProcessPass = std::make_shared<PostProcessPass2D>();
    m_uiPass = std::make_shared<UIPass2D>();
    return 0;
}

void NPRPipeline::Shutdown() {
    m_nprOpaquePass.reset();
    m_skyboxPass.reset();
    m_postProcessPass.reset();
    m_uiPass.reset();
}

void NPRPipeline::Execute(const RenderContext& ctx) {
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

    // ── NPR Opaque Pass ──
    if (m_nprOpaquePass) {
        m_nprOpaquePass->SetViewMatrix(view);
        m_nprOpaquePass->SetProjectionMatrix(proj);
        m_nprOpaquePass->SetLights(ctx.lights);
        m_nprOpaquePass->Execute(passContext);
        if (ctx.commandBuffer) {
            m_nprOpaquePass->Execute(ctx.commandBuffer, commands);
        }
    }

    // ── Skybox Pass ──
    if (m_skyboxPass) {
        m_skyboxPass->SetViewMatrix(view);
        m_skyboxPass->SetProjectionMatrix(proj);
        m_skyboxPass->Execute(passContext);
    }

    // ── Post Processing ──
    if (m_postProcessPass && ctx.commandBuffer) {
        // m_postProcessPass->Process(ctx.commandBuffer, ctx.device, sceneResultTexture);
    }

    // ── UI Pass ──
    if (m_uiPass && ctx.commandBuffer) {
        m_uiPass->RenderUI(ctx.commandBuffer, ctx.device, ctx.width, ctx.height);
    }
}

} // namespace Prisma::Graphic
