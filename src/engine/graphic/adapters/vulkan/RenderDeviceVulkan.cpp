// [修复] 不在此包含 imgui_impl_vulkan.h
// 目的：Engine.dll 不再直接使用 ImGui Vulkan 后端 API，
//         渲染通过由 PrismaEditor.dll 注册的回调执行，
//         避免两份独立的编译单元共享状态导致指针崩溃。
#include "RenderDeviceVulkan.h"
#include "Logger.h"
#include "VulkanFence.h"
#include "VulkanResourceFactory.h"
#include "VulkanSwapChain.h"
#include <iostream>

#define VMA_IMPLEMENTATION
#include <SDL3/SDL_vulkan.h>
#include <vk_mem_alloc.h>


#include "Engine.h"

namespace Prisma::Graphic::Vulkan {

RenderDeviceVulkan::RenderDeviceVulkan() {
    LOG_INFO("Vulkan", "创建 Vulkan 渲染设备实例");
    m_resourceFactory = std::make_unique<VulkanResourceFactory>(this);
    m_swapChain       = std::make_unique<VulkanSwapChain>(this);
    LOG_INFO("Vulkan", "Vulkan 渲染设备实例创建成功");
}

RenderDeviceVulkan::~RenderDeviceVulkan() {
    Shutdown();
}

int RenderDeviceVulkan::Initialize(const DeviceDesc& desc) {
    m_desc = desc;
    LOG_INFO("Vulkan", "正在初始化 Vulkan 设备");

    try {
        // 1. 创建实例
        vkb::InstanceBuilder inst_builder;
        auto inst_ret = inst_builder.set_app_name(desc.name.c_str())
                            .request_validation_layers(desc.enableValidation)
                            .use_default_debug_messenger()
                            .require_api_version(1, 3, 0)
                            .build();

        if (!inst_ret)
            return -1;
        m_vkbInstance = inst_ret.value();
        m_instance    = m_vkbInstance.instance;

        auto& window          = Engine::Get().GetWindow();
        SDL_Window* sdlWindow = static_cast<SDL_Window*>(window.GetNativeWindow());
        if (!SDL_Vulkan_CreateSurface(sdlWindow, m_instance, nullptr, &m_surface)) {
            return -1;
        }

        // 2. 选择物理设备
        vkb::PhysicalDeviceSelector selector{m_vkbInstance};
        auto phys_ret = selector.set_surface(m_surface)
                            .set_minimum_version(1, 3)
                            .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
                            .select();
        if (!phys_ret)
            return -2;
        m_vkbPhysicalDevice = phys_ret.value();
        m_physicalDevice    = m_vkbPhysicalDevice.physical_device;

        // 3. 创建逻辑设备
        vkb::DeviceBuilder device_builder{m_vkbPhysicalDevice};
        auto dev_ret = device_builder.build();
        if (!dev_ret)
            return -3;
        m_vkbDevice = dev_ret.value();
        m_device    = m_vkbDevice.device;

        // 4. 获取队列
        m_graphicsQueue       = m_vkbDevice.get_queue(vkb::QueueType::graphics).value();
        m_graphicsQueueFamily = m_vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

        // 5. 初始化 VMA
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.vulkanApiVersion       = VK_API_VERSION_1_3;
        allocatorInfo.physicalDevice         = m_physicalDevice;
        allocatorInfo.device                 = m_device;
        allocatorInfo.instance               = m_instance;
        if (vmaCreateAllocator(&allocatorInfo, &m_allocator) != VK_SUCCESS)
            return -5;

        // 5.1 同步资源工厂的分销器
        if (m_resourceFactory) {
            m_resourceFactory->Initialize(this);
        }



        // 7. Command Pool & Buffers
        VkCommandPoolCreateInfo cmd_pool_info = {};
        cmd_pool_info.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmd_pool_info.queueFamilyIndex        = m_graphicsQueueFamily;
        cmd_pool_info.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        vkCreateCommandPool(m_device, &cmd_pool_info, nullptr, &m_commandPool);

        m_commandBuffers.resize(3);
        VkCommandBufferAllocateInfo cmd_alloc_info = {};
        cmd_alloc_info.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd_alloc_info.commandPool                 = m_commandPool;
        cmd_alloc_info.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmd_alloc_info.commandBufferCount          = 3;
        vkAllocateCommandBuffers(m_device, &cmd_alloc_info, m_commandBuffers.data());

        // 8. Sync Objects
        m_imageAvailableSemaphores.resize(3);
        m_renderFinishedSemaphores.resize(3);  // 这里后面会根据交换链调整
        m_inFlightFences.resize(3);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < 3; i++) {
            vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]);
            vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]);
            vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]);
        }

        // 9. SwapChain
        m_swapChain->Initialize(m_surface, desc.width, desc.height, desc.vsync);

        // 确保 renderFinishedSemaphores 足够大
        uint32_t imageCount = m_swapChain->GetBufferCount();
        if (m_renderFinishedSemaphores.size() < imageCount) {
            size_t oldSize = m_renderFinishedSemaphores.size();
            m_renderFinishedSemaphores.resize(imageCount);
            for (size_t i = oldSize; i < imageCount; i++) {
                vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]);
            }
        }

        m_initialized = true;
        return 0;
    } catch (...) {
        return -999;
    }
}

void RenderDeviceVulkan::Shutdown() {
    if (!m_initialized)
        return;
    
    // 确保 GPU 已完成所有工作
    if (m_device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(m_device);
    }

    // 1. 先销毁由此设备管理的子资源
    if (m_resourceFactory) {
        m_resourceFactory->Shutdown();
        m_resourceFactory.reset();
    }

    if (m_swapChain) {
        m_swapChain->Cleanup();
        m_swapChain.reset();
    }

    // 2. 销毁同步对象和命令池
    for (auto s : m_imageAvailableSemaphores)
        vkDestroySemaphore(m_device, s, nullptr);
    for (auto s : m_renderFinishedSemaphores)
        vkDestroySemaphore(m_device, s, nullptr);
    for (auto f : m_inFlightFences)
        vkDestroyFence(m_device, f, nullptr);

    if (m_commandPool)
        vkDestroyCommandPool(m_device, m_commandPool, nullptr);

    // 3. 销毁基础组件
    if (m_allocator) {
        vmaDestroyAllocator(m_allocator);
        m_allocator = VK_NULL_HANDLE;
    }

    if (m_surface) {
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        m_surface = VK_NULL_HANDLE;
    }

    // 4. 最后销毁设备和实例
    vkb::destroy_device(m_vkbDevice);
    vkb::destroy_instance(m_vkbInstance);
    
    m_device = VK_NULL_HANDLE;
    m_instance = VK_NULL_HANDLE;
    m_initialized = false;
}

void RenderDeviceVulkan::BeginFrame() {
    if (!m_initialized)
        return;
    vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

    if (!m_swapChain->AcquireNextImage(m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE)) {
        return;
    }

    vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]);

    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmd, &beginInfo);

    // [改动] 统一保持 m_frameActive 为 true
    // 目的：即使跳过了默认 RenderPass，命令缓冲区依然在录制（由调用方负责开启自己的 RenderPass），
    //       必须保持活动状态以确保 EndFrame 能够执行提交。
    m_frameActive = true;

    // 如果不跳过交换链RenderPass，则开始它
    if (!m_skipSwapChainRenderPass) {
        VkRenderPassBeginInfo rpInfo{};
        rpInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpInfo.renderPass        = m_swapChain->GetRenderPass();
        rpInfo.framebuffer       = m_swapChain->GetCurrentFramebuffer();
        rpInfo.renderArea.extent = m_swapChain->GetExtent();
        VkClearValue clearColor  = {{{0.1f, 0.1f, 0.1f, 1.0f}}};
        rpInfo.clearValueCount   = 1;
        rpInfo.pClearValues      = &clearColor;

        vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
        m_isDefaultRenderPassActive = true;
    } else {
        // 重置标志，下一帧恢复默认行为
        m_skipSwapChainRenderPass = false;
        m_isDefaultRenderPassActive = false;
    }
    m_currentFrameIndex = m_currentFrame;
    m_hasPendingPresent = false;
}

void RenderDeviceVulkan::EndFrame() {
    if (!m_initialized || !m_frameActive)
        return;
    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];

    // [修复] 如果还没有开启 RenderPass (说明之前被跳过了)，现在为了 Overlay 开启它。
    // 这样可以确保 ImGui 的绘制指令处于合法的 RenderPass 中，
    // 同时通过 RenderPass 的 finalLayout 自动将交换链图像转换到 PRESENT_SRC_KHR 布局，
    // 彻底解决 VUID-vkCmdDrawIndexed-renderpass 和 VUID-VkPresentInfoKHR-pImageIndices-01430。
    if (!m_isDefaultRenderPassActive) {
        VkRenderPassBeginInfo rpInfo{};
        rpInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpInfo.renderPass        = m_swapChain->GetRenderPass();
        rpInfo.framebuffer       = m_swapChain->GetCurrentFramebuffer();
        rpInfo.renderArea.extent = m_swapChain->GetExtent();
        VkClearValue clearColor  = {{{0.1f, 0.1f, 0.1f, 1.0f}}};
        rpInfo.clearValueCount   = 1;
        rpInfo.pClearValues      = &clearColor;

        vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
        m_isDefaultRenderPassActive = true;
    }

    if (m_overlayRenderCallback) {
        m_overlayRenderCallback(cmd);
    }

    // 无论如何都要结束活动中的默认 RenderPass
    if (m_isDefaultRenderPassActive) {
        vkCmdEndRenderPass(cmd);
        m_isDefaultRenderPassActive = false;
    }

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{};
    submitInfo.sType                  = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount     = 1;
    submitInfo.pWaitSemaphores        = &m_imageAvailableSemaphores[m_currentFrame];
    submitInfo.pWaitDstStageMask      = waitStages;
    submitInfo.commandBufferCount     = 1;
    submitInfo.pCommandBuffers        = &cmd;

    uint32_t imageIndex             = m_swapChain->GetCurrentBufferIndex();
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = &m_renderFinishedSemaphores[imageIndex];

    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]);
    m_pendingPresentImageIndex = imageIndex;
    m_hasPendingPresent        = true;
    m_frameActive              = false;
}

void RenderDeviceVulkan::Present() {
    if (!m_initialized || !m_hasPendingPresent || !m_swapChain) {
        return;
    }

    if (!m_swapChain->Present(m_renderFinishedSemaphores[m_pendingPresentImageIndex])) {
        LOG_WARNING("Vulkan", "Swapchain present returned suboptimal or out-of-date");
    }

    m_hasPendingPresent = false;
    m_currentFrame      = (m_currentFrame + 1) % static_cast<uint32_t>(m_commandBuffers.size());
}
void RenderDeviceVulkan::Resize(uint32_t width, uint32_t height) {
    if (m_device)
        vkDeviceWaitIdle(m_device);
    if (m_swapChain) {
        m_swapChain->Resize(width, height);
    }
    m_frameActive       = false;
    m_hasPendingPresent = false;
}

std::unique_ptr<ICommandBuffer> RenderDeviceVulkan::CreateCommandBuffer(CommandBufferType type) {
    LOG_WARNING(
        "Vulkan", "CreateCommandBuffer is not implemented for command buffer type: {0}", static_cast<int>(type));
    return nullptr;
}
void RenderDeviceVulkan::SubmitCommandBuffer(ICommandBuffer* cmdBuffer, IFence* fence) {
    if (!cmdBuffer) {
        LOG_WARNING("Vulkan", "SubmitCommandBuffer called with null command buffer");
        return;
    }

    LOG_WARNING("Vulkan", "Standalone command buffer submission is not implemented; use the frame command buffer path");
    if (fence) {
        fence->Signal(1);
    }
}
void RenderDeviceVulkan::SubmitCommandBuffers(const std::vector<ICommandBuffer*>& cmdBuffers,
                                              const std::vector<IFence*>& fences) {
    for (auto* cmdBuffer : cmdBuffers) {
        SubmitCommandBuffer(cmdBuffer, nullptr);
    }
    for (auto* fence : fences) {
        if (fence) {
            fence->Signal(1);
        }
    }
}
void RenderDeviceVulkan::WaitForIdle() {
    if (m_device)
        vkDeviceWaitIdle(m_device);
}
std::unique_ptr<IFence> RenderDeviceVulkan::CreateFence() {
    if (m_device == VK_NULL_HANDLE) {
        return nullptr;
    }
    return std::make_unique<VulkanFence>(m_device);
}
void RenderDeviceVulkan::WaitForFence(IFence* fence) {
    if (fence) {
        fence->Wait(1);
    }
}
IResourceFactory* RenderDeviceVulkan::GetResourceFactory() const {
    return m_resourceFactory.get();
}
std::unique_ptr<ISwapChain>
RenderDeviceVulkan::CreateSwapChain(void* windowHandle, uint32_t width, uint32_t height, bool vsync) {
    auto swapChain = std::make_unique<VulkanSwapChain>(this);
    if (swapChain->Initialize(windowHandle, width, height, vsync) != 0) {
        return nullptr;
    }
    return swapChain;
}
ISwapChain* RenderDeviceVulkan::GetSwapChain() const {
    return m_swapChain.get();
}
IRenderDevice::GPUMemoryInfo RenderDeviceVulkan::GetGPUMemoryInfo() const {
    return {};
}
IRenderDevice::RenderStats RenderDeviceVulkan::GetRenderStats() const {
    return m_stats;
}
void RenderDeviceVulkan::BeginDebugMarker(const std::string& name) {
    LOG_DEBUG("Vulkan", "开始调试标记: {0}", name);
}
void RenderDeviceVulkan::EndDebugMarker() {}
void RenderDeviceVulkan::SetDebugMarker(const std::string& name) {
    LOG_DEBUG("Vulkan", "设置调试标记: {0}", name);
}
std::string RenderDeviceVulkan::GetName() const {
    return "Vulkan Device";
}
std::string RenderDeviceVulkan::GetAPIName() const {
    return "Vulkan";
}
VkRenderPass RenderDeviceVulkan::GetOverlayRenderPass() const {
    return m_swapChain ? m_swapChain->GetRenderPass() : VK_NULL_HANDLE;
}

}  // namespace Prisma::Graphic::Vulkan
