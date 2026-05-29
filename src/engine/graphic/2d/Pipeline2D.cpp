#include "Pipeline2D.h"
#include "CanvasPass2D.h"
#include "Light2DPass.h"
#include "PixelPerfectPass.h"
#include "UIPass2D.h"
#include "PostProcessPass2D.h"
#include "graphic/Renderer.h"
#include "graphic/Renderer2D.h"
#include "graphic/RenderCommandContext.h"
#include "graphic/pipelines/forward/OpaquePass.h"
#include "adapters/vulkan/RenderDeviceVulkan.h"
#include "adapters/vulkan/VulkanCommandBuffer.h"
#include "adapters/vulkan/VulkanResources.h"
#include "Logger.h"

namespace Prisma::Graphic {

Pipeline2D::Pipeline2D() = default;

Pipeline2D::~Pipeline2D() {
    Shutdown();
}

int Pipeline2D::Initialize(IRenderDevice* device) {
    m_device = device;
    m_lightPass = std::make_shared<Light2DPass>();
    m_opaquePass = std::make_shared<OpaquePass>();
    m_opaquePass->SetDevice(device);
    m_canvasPass = std::make_shared<CanvasPass2D>();
    m_pixelPass = std::make_shared<PixelPerfectPass>();
    m_pixelPass->Initialize(device);

    // 使用 VulkanCommandBuffer 缓存机制创建离屏 RenderPass
    // 确保 BeginRenderPass 使用与 PSO 创建相同的 VkRenderPass 对象
    {
        auto* vkDevice = dynamic_cast<Vulkan::RenderDeviceVulkan*>(device);
        auto* offscreenTex = dynamic_cast<Vulkan::VulkanTexture*>(m_pixelPass->GetOffscreenTexture());
        auto* depthTex = dynamic_cast<Vulkan::VulkanTexture*>(m_pixelPass->GetDepthTexture());
        if (vkDevice && offscreenTex) {
            VkRenderPass rp = Vulkan::VulkanCommandBuffer::PreCreateOffscreenRenderPass(
                vkDevice->GetVkDevice(),
                offscreenTex->GetVkImageView(), offscreenTex->GetVkFormat(),
                depthTex ? depthTex->GetVkImageView() : VK_NULL_HANDLE,
                depthTex ? depthTex->GetVkFormat() : VK_FORMAT_D32_SFLOAT,
                static_cast<uint32_t>(offscreenTex->GetWidth()),
                static_cast<uint32_t>(offscreenTex->GetHeight()),
                true, true);
            if (rp != VK_NULL_HANDLE) {
                m_opaquePass->CreatePipelineForRenderPass(rp);
                LOG_INFO("Pipeline2D", "离屏 RenderPass 已创建并传递给 OpaquePass");
            } else {
                LOG_ERROR("Pipeline2D", "创建离屏 RenderPass 失败");
            }
        } else {
            LOG_WARN("Pipeline2D", "非 Vulkan 设备或无离屏纹理，跳过离屏 RenderPass 创建");
        }
    }
    m_ppPass = std::make_shared<PostProcessPass2D>();
    m_uiPass = std::make_shared<UIPass2D>();

    LOG_INFO("Pipeline2D", "纯 2D 渲染管线初始化完成");
    return 0;
}

void Pipeline2D::Shutdown() {
    if (m_pixelPass) {
        m_pixelPass->Shutdown();
        m_pixelPass.reset();
    }
    m_lightPass.reset();
    m_opaquePass.reset();
    m_canvasPass.reset();
    m_ppPass.reset();
    m_uiPass.reset();

}

void Pipeline2D::Execute(const RenderContext& ctx) {
    if (!m_device || !ctx.commandBuffer) return;

    auto view = ctx.camera.viewMatrix;
    auto proj = ctx.camera.projectionMatrix;

    // ── 判断是否启用 PixelPerfect 模式 ──
    // 仅在直接渲染到交换链时启用（非 ctx.targetTexture 目标）
    bool usePixelPerfect = m_pixelPass &&
                           m_pixelPass->IsPixelPerfectEnabled() &&
                           m_pixelPass->GetOffscreenTexture() != nullptr &&
                           !ctx.targetTexture;

    // ═══════════════════════════════════════════════════════════════
    // 阶段 0: 短暂开始并结束交换链 RP（仅为了清除颜色 / depth）
    //         之后光照 Pass 在离屏纹理上运行
    // ═══════════════════════════════════════════════════════════════
    if (!ctx.targetTexture) {
        ctx.device->BeginSwapChainRenderPass(ctx.clearColor);
        ctx.device->EndSwapChainRenderPass();
    }

    // ── 1. 2D 光照预处理 (离屏) ──
    if (m_lightPass) {
        m_lightPass->SetViewMatrix(view);
        m_lightPass->SetProjectionMatrix(proj);
        m_lightPass->ExecuteLight(ctx.commandBuffer, m_device, ctx.width, ctx.height);
        // 将光照纹理传递给 Renderer2D
        Renderer2D::SetLightTexture(m_lightPass->GetLightTexture());
    }

    // ═══════════════════════════════════════════════════════════════
    // 阶段 1: 主内容渲染（Opaque + Canvas）
    //         启用 PixelPerfect 时渲染到离屏 RT，否则渲染到交换链
    // ═══════════════════════════════════════════════════════════════
    if (usePixelPerfect) {
        // ── 开始离屏 RenderPass (256×224) ──
        RenderPassDesc rpDesc;
        rpDesc.renderTarget = m_pixelPass->GetOffscreenTexture();
        rpDesc.depthStencil = m_pixelPass->GetDepthTexture();
        {
            const float* cc = m_pixelPass->GetClearColorArray();
            rpDesc.clearColor = { cc[0], cc[1], cc[2], cc[3] };
        }
        rpDesc.clearRenderTarget = true;
        rpDesc.clearDepth = true;
        rpDesc.renderArea = {
            0, 0,
            static_cast<int>(m_pixelPass->GetLogicalWidth()),
            static_cast<int>(m_pixelPass->GetLogicalHeight())
        };
        ctx.commandBuffer->BeginRenderPass(rpDesc);

        // 设置视口为离屏尺寸
        Viewport vp;
        vp.x = 0.0f;
        vp.y = 0.0f;
        vp.width = static_cast<float>(m_pixelPass->GetLogicalWidth());
        vp.height = static_cast<float>(m_pixelPass->GetLogicalHeight());
        vp.minDepth = 0.0f;
        vp.maxDepth = 1.0f;
        ctx.commandBuffer->SetViewport(vp);

        Rect scissor;
        scissor.x = 0;
        scissor.y = 0;
        scissor.width = static_cast<int>(m_pixelPass->GetLogicalWidth());
        scissor.height = static_cast<int>(m_pixelPass->GetLogicalHeight());
        ctx.commandBuffer->SetScissorRect(scissor);
    } else if (!ctx.targetTexture) {
        ctx.device->BeginSwapChainRenderPass(ctx.clearColor);
    } else if (ctx.targetTexture) {
        RenderPassDesc rpDesc;
        rpDesc.renderTarget = ctx.targetTexture;
        rpDesc.clearRenderTarget = false;
        rpDesc.clearDepth = false;
        ctx.commandBuffer->BeginRenderPass(rpDesc);
    }

    // ── 2. 渲染 Renderer2D 内容 (Sprite batching) ──
    const auto& commands = Renderer::GetCommandQueue();
    if (!commands.empty() && m_opaquePass) {
        m_opaquePass->SetViewMatrix(view);
        m_opaquePass->SetProjectionMatrix(proj);
        m_opaquePass->SetUseOffscreenPipeline(usePixelPerfect);
        m_opaquePass->Execute(ctx.commandBuffer, commands);
    }

    // ── 3. 世界空间 Canvas 渲染 (Graphics2D) ──
    if (m_canvasPass) {
        m_canvasPass->Render(ctx.commandBuffer, m_device);
    }

    if (usePixelPerfect) {
        ctx.commandBuffer->EndRenderPass();
    } else if (!ctx.targetTexture) {
        ctx.device->EndSwapChainRenderPass();
    } else {
        ctx.commandBuffer->EndRenderPass();
    }

    // ═══════════════════════════════════════════════════════════════
    // 阶段 2: 离屏 → 交换链 (PixelPerfect Blit / CRT 后处理)
    //         仅在 PixelPerfect 或 CRT 模式下需要，直接渲染到交换链
    //         时跳过此阶段，避免 LOAD_OP_CLEAR 清除阶段 1 的内容。
    // ═══════════════════════════════════════════════════════════════
    if (!ctx.targetTexture) {
        bool crtEnabled = m_ppPass &&
            m_ppPass->IsEffectEnabled(PostProcessPass2D::EffectType::CRT);

        if (usePixelPerfect) {
            ctx.device->BeginSwapChainRenderPass(ctx.clearColor);

            if (crtEnabled) {
                // CRT 模式：CRT 着色器同时处理上采样 + CRT 效果
                m_ppPass->Process(ctx.commandBuffer, m_device,
                                  m_pixelPass->GetOffscreenTexture(), nullptr);
            } else {
                // 普通模式：简单整数倍最近邻上采样
                m_pixelPass->BlitToSwapChain(ctx.commandBuffer, m_device, nullptr);
            }
        }
    }

    // ── 4. 2D 后处理（非 PixelPerfect 模式下的后处理暂不启用） ──
    // 当使用标准 2D 渲染（非像素完美模式）时，未来可在此处添加后处理

    // ── 5. 屏幕空间 UI 渲染 ──
    if (m_uiPass) {
        m_uiPass->RenderUI(ctx.commandBuffer, m_device, ctx.width, ctx.height);
    }
}

} // namespace Prisma::Graphic
