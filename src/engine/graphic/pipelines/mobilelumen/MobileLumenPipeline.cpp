#include "MobileLumenPipeline.h"
#include "../ProceduralSkyPass.h"
#include "../forward/DepthPrePass.h"
#include "../forward/OpaquePass.h"
#include "graphic/Renderer.h"
#include "graphic/RenderCommandContext.h"
#include "graphic/adapters/vulkan/VulkanResources.h"
#include "interfaces/IDeviceContext.h"
#include "interfaces/IPass.h"
#include "interfaces/IRenderDevice.h"
#include "interfaces/IRenderTarget.h"
#include "interfaces/ICommandBuffer.h"
#include "logger/Logger.h"
#include "math/MathTypes.h"

namespace Prisma::Graphic {

// [复制自 ForwardPipeline.cpp:38-69] 内部渲染目标代理
// 匿名 namespace 保证内部链接,避免与 ForwardPipeline.cpp 的同名类冲突
namespace {
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
} // namespace

MobileLumenPipeline::~MobileLumenPipeline() { Shutdown(); }

int MobileLumenPipeline::Initialize(IRenderDevice* device) {
    if (!device) {
        LOG_ERROR("MobileLumenPipeline", "Initialize 失败:device 为空");
        return -1;
    }
    m_device = device;

    m_depthPrePass = std::make_shared<DepthPrePass>();
    m_opaquePass = std::make_shared<OpaquePass>();
    m_opaquePass->SetDevice(device);

    m_skyPass = std::make_shared<ProceduralSkyPass>();
    if (!m_skyPass->Initialize(device)) {
        LOG_WARNING("MobileLumenPipeline", "ProceduralSkyPass 初始化失败,天空将不渲染");
        m_skyPass.reset();
    }

    m_initialized = true;
    LOG_INFO("MobileLumenPipeline", "MobileLumen 管线初始化完成 (DepthPrePass + OpaquePass + ProceduralSky)");
    return 0;
}

void MobileLumenPipeline::Shutdown() {
    if (!m_initialized) return;
    if (m_skyPass) { m_skyPass->Shutdown(); m_skyPass.reset(); }
    m_opaquePass.reset();
    m_depthPrePass.reset();
    m_device = nullptr;
    m_initialized = false;
    LOG_INFO("MobileLumenPipeline", "MobileLumen 管线关闭");
}

void MobileLumenPipeline::Execute(const RenderContext& ctx) {
    if (!m_initialized || !ctx.device) return;

    // 1. Begin swapchain render pass (离屏模式跳过)
    if (!ctx.targetTexture) {
        ctx.device->BeginSwapChainRenderPass(ctx.clearColor);
    }

    TextureRenderTargetProxy proxy(ctx.targetTexture);

    const auto& commands = Renderer::GetCommandQueue();
    auto view = ctx.camera.viewMatrix;
    auto proj = ctx.camera.projectionMatrix;

    RenderCommandContext fallbackContext;
    IDeviceContext* deviceContext = &fallbackContext;

    SceneData sceneData;
    sceneData.camera.view = view;
    sceneData.camera.projection = proj;
    sceneData.camera.viewProjection = proj * view;
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

    // 2. MLGI update hook (Phase 2 预留)
    UpdateGI(ctx);

    // 3. Depth pre-pass
    if (m_depthPrePass) {
        m_depthPrePass->SetViewMatrix(view);
        m_depthPrePass->SetProjectionMatrix(proj);
        m_depthPrePass->Execute(passContext);
    }

    // 4. Opaque pass
    if (m_opaquePass) {
        m_opaquePass->SetViewMatrix(view);
        m_opaquePass->SetProjectionMatrix(proj);
        m_opaquePass->SetLights(ctx.lights);
        if (ctx.commandBuffer) {
            m_opaquePass->Execute(ctx.commandBuffer, commands);
        }
    }

    // 5. Procedural sky pass (从方向光提取太阳参数)
    if (m_skyPass) {
        m_skyPass->SetViewMatrix(view);
        m_skyPass->SetProjectionMatrix(proj);
        for (const auto& light : ctx.lights) {
            if (light.direction.w < 0.5f) {  // 方向光
                m_skyPass->SetSunDirection(PrismaMath::vec3(light.direction.x,
                                                            light.direction.y,
                                                            light.direction.z));
                m_skyPass->SetSunColor(PrismaMath::vec3(light.color.x,
                                                        light.color.y,
                                                        light.color.z));
                m_skyPass->SetSunIntensity(light.color.w);
                break;
            }
        }
        m_skyPass->Execute(passContext);
    }

    // 6. End swapchain render pass (离屏模式跳过)
    if (!ctx.targetTexture) {
        ctx.device->EndSwapChainRenderPass();
    }
}

} // namespace Prisma::Graphic
