#include "ForwardPipeline.h"
#include "DepthPrePass.h"
#include "OpaquePass.h"
#include "TransparentPass.h"
#include "../SkyboxRenderPass.h"
#include "graphic/Renderer.h"
#include "graphic/RenderCommandContext.h"
#include "Logger.h"

// Vulkan 特定代码支持
#include "adapters/vulkan/RenderDeviceVulkan.h"
#include "adapters/vulkan/VulkanResources.h"
#include "graphic/interfaces/IRenderTarget.h"

namespace Prisma::Graphic {

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
    m_skyboxPass = std::make_shared<SkyboxPass>();
    m_transparentPass = std::make_shared<TransparentPass>();
    return 0;
}

void ForwardPipeline::Shutdown() {
    m_depthPrePass.reset();
    m_opaquePass.reset();
    m_skyboxPass.reset();
    m_transparentPass.reset();
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
}

} // namespace Prisma::Graphic
