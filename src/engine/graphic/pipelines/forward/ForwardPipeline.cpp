#include "ForwardPipeline.h"
#include "DepthPrePass.h"
#include "OpaquePass.h"
#include "TransparentPass.h"
#include "../SkyboxRenderPass.h"
#include "graphic/Renderer.h"
#include "graphic/RenderCommandContext.h"
#include "Logger.h"

// Vulkan 特定代码，用于渲染到纹理
#include "adapters/vulkan/RenderDeviceVulkan.h"
#include "adapters/vulkan/VulkanResources.h"
#include <vulkan/vulkan.h>

namespace Prisma::Graphic {

ForwardPipeline::ForwardPipeline() = default;

ForwardPipeline::~ForwardPipeline() {
    Shutdown();
}

int ForwardPipeline::Initialize(IRenderDevice* device) {
    m_device = device;
    m_depthPrePass = std::make_shared<DepthPrePass>();
    m_opaquePass = std::make_shared<OpaquePass>();
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

    // 调试：检查是否有目标纹理
    if (ctx.targetTexture) {
        LOG_INFO("ForwardPipeline", "Rendering to target texture: {}x{}", ctx.width, ctx.height);

        // 尝试获取Vulkan设备并设置跳过交换链RenderPass
        auto vulkanDevice = dynamic_cast<Vulkan::RenderDeviceVulkan*>(ctx.device);
        auto vulkanTexture = dynamic_cast<Vulkan::VulkanTexture*>(ctx.targetTexture);
        if (vulkanDevice && vulkanTexture) {
            vulkanDevice->SetSkipSwapChainRenderPass(true);
            LOG_INFO("ForwardPipeline", "Set skip swapchain render pass");

            // 尝试创建离屏渲染
            VkCommandBuffer cmd = vulkanDevice->GetCurrentCommandBuffer();
            if (cmd != VK_NULL_HANDLE) {
                // 获取交换链的RenderPass（可能不兼容，但先尝试）
                VkRenderPass renderPass = vulkanDevice->GetOverlayRenderPass();
                VkImageView imageView = vulkanTexture->GetVkImageView();

                if (renderPass != VK_NULL_HANDLE && imageView != VK_NULL_HANDLE) {
                    // 创建Framebuffer
                    VkFramebufferCreateInfo fbInfo = {};
                    fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                    fbInfo.renderPass = renderPass;
                    fbInfo.attachmentCount = 1;
                    fbInfo.pAttachments = &imageView;
                    fbInfo.width = static_cast<uint32_t>(ctx.width);
                    fbInfo.height = static_cast<uint32_t>(ctx.height);
                    fbInfo.layers = 1;

                    VkFramebuffer framebuffer = VK_NULL_HANDLE;
                    VkResult result = vkCreateFramebuffer(vulkanDevice->GetVkDevice(), &fbInfo, nullptr, &framebuffer);
                    if (result == VK_SUCCESS) {
                        // 开始RenderPass
                        VkRenderPassBeginInfo rpInfo = {};
                        rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                        rpInfo.renderPass = renderPass;
                        rpInfo.framebuffer = framebuffer;
                        rpInfo.renderArea.extent = { static_cast<uint32_t>(ctx.width), static_cast<uint32_t>(ctx.height) };
                        VkClearValue clearColor = {{{0.0f, 0.5f, 1.0f, 1.0f}}}; // 蓝色
                        rpInfo.clearValueCount = 1;
                        rpInfo.pClearValues = &clearColor;

                        vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

                        // 这里可以添加实际的渲染命令
                        // 暂时只是清除颜色

                        vkCmdEndRenderPass(cmd);

                        // 销毁Framebuffer（应该缓存，但先这样）
                        vkDestroyFramebuffer(vulkanDevice->GetVkDevice(), framebuffer, nullptr);

                        LOG_INFO("ForwardPipeline", "Created offscreen render pass and cleared texture");
                    } else {
                        LOG_ERROR("ForwardPipeline", "Failed to create framebuffer: {}", (int)result);
                    }
                }
            }
        } else {
            LOG_WARNING("ForwardPipeline", "Cannot render to texture: device or texture is not Vulkan");
        }
    } else {
        LOG_INFO("ForwardPipeline", "Rendering to swapchain: {}x{}", ctx.width, ctx.height);
    }

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
