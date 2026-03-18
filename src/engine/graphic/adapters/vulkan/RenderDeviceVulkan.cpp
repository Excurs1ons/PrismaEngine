#define IMGUI_IMPL_VULKAN_USE_LOADER
#include "RenderDeviceVulkan.h"
#include "Logger.h"
#include "VulkanResourceFactory.h"
#include "VulkanSwapChain.h"
#include <iostream>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#include <imgui_impl_vulkan.h>
#include <SDL3/SDL_vulkan.h>

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

        if (!inst_ret) return -1;
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
        if (!phys_ret) return -2;
        m_vkbPhysicalDevice = phys_ret.value();
        m_physicalDevice    = m_vkbPhysicalDevice.physical_device;

        // 3. 创建逻辑设备
        vkb::DeviceBuilder device_builder{m_vkbPhysicalDevice};
        auto dev_ret = device_builder.build();
        if (!dev_ret) return -3;
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
        if (vmaCreateAllocator(&allocatorInfo, &m_allocator) != VK_SUCCESS) return -5;

        // 6. ImGui Descriptor Pool
        VkDescriptorPoolSize pool_sizes[] = {
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000}
        };
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000;
        pool_info.poolSizeCount = 1;
        pool_info.pPoolSizes = pool_sizes;
        vkCreateDescriptorPool(m_device, &pool_info, nullptr, &m_imguiDescriptorPool);

        // 7. Command Pool & Buffers
        VkCommandPoolCreateInfo cmd_pool_info = {};
        cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmd_pool_info.queueFamilyIndex = m_graphicsQueueFamily;
        cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        vkCreateCommandPool(m_device, &cmd_pool_info, nullptr, &m_commandPool);

        m_commandBuffers.resize(3);
        VkCommandBufferAllocateInfo cmd_alloc_info = {};
        cmd_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd_alloc_info.commandPool = m_commandPool;
        cmd_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmd_alloc_info.commandBufferCount = 3;
        vkAllocateCommandBuffers(m_device, &cmd_alloc_info, m_commandBuffers.data());

        // 8. Sync Objects
        m_imageAvailableSemaphores.resize(3);
        m_renderFinishedSemaphores.resize(3); // 这里后面会根据交换链调整
        m_inFlightFences.resize(3);

        VkSemaphoreCreateInfo semaphoreInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VkFenceCreateInfo fenceInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
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
    if (!m_initialized) return;
    vkDeviceWaitIdle(m_device);

    for (auto s : m_imageAvailableSemaphores) vkDestroySemaphore(m_device, s, nullptr);
    for (auto s : m_renderFinishedSemaphores) vkDestroySemaphore(m_device, s, nullptr);
    for (auto f : m_inFlightFences) vkDestroyFence(m_device, f, nullptr);

    if (m_commandPool) vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    if (m_imguiDescriptorPool) vkDestroyDescriptorPool(m_device, m_imguiDescriptorPool, nullptr);
    if (m_swapChain) m_swapChain->Cleanup();
    if (m_allocator) vmaDestroyAllocator(m_allocator);
    if (m_surface) vkDestroySurfaceKHR(m_instance, m_surface, nullptr);

    vkb::destroy_device(m_vkbDevice);
    vkb::destroy_instance(m_vkbInstance);
    m_initialized = false;
}

void RenderDeviceVulkan::BeginFrame() {
    if (!m_initialized) return;
    vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);
    
    if (!m_swapChain->AcquireNextImage(m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE)) return;
    
    vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]);

    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    vkBeginCommandBuffer(cmd, &beginInfo);

    VkRenderPassBeginInfo rpInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rpInfo.renderPass = m_swapChain->GetRenderPass();
    rpInfo.framebuffer = m_swapChain->GetCurrentFramebuffer();
    rpInfo.renderArea.extent = m_swapChain->GetExtent();
    VkClearValue clearColor = {{{0.1f, 0.1f, 0.1f, 1.0f}}};
    rpInfo.clearValueCount = 1;
    rpInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void RenderDeviceVulkan::EndFrame() {
    if (!m_initialized) return;
    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];
    
    if (ImGui::GetCurrentContext() && ImGui::GetDrawData()) {
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
    }

    vkCmdEndRenderPass(cmd);
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &m_imageAvailableSemaphores[m_currentFrame];
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    uint32_t imageIndex = m_swapChain->GetCurrentBufferIndex();
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &m_renderFinishedSemaphores[imageIndex];

    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]);
    m_swapChain->Present(m_renderFinishedSemaphores[imageIndex]);

    m_currentFrame = (m_currentFrame + 1) % 3;
}

void RenderDeviceVulkan::Present() {}
void RenderDeviceVulkan::Resize(uint32_t width, uint32_t height) {
    if (m_device) vkDeviceWaitIdle(m_device);
    if (m_swapChain) m_swapChain->Resize(width, height);
}

std::unique_ptr<ICommandBuffer> RenderDeviceVulkan::CreateCommandBuffer(CommandBufferType type) { return nullptr; }
void RenderDeviceVulkan::SubmitCommandBuffer(ICommandBuffer* cmdBuffer, IFence* fence) {}
void RenderDeviceVulkan::SubmitCommandBuffers(const std::vector<ICommandBuffer*>& cmdBuffers, const std::vector<IFence*>& fences) {}
void RenderDeviceVulkan::WaitForIdle() { if (m_device) vkDeviceWaitIdle(m_device); }
std::unique_ptr<IFence> RenderDeviceVulkan::CreateFence() { return nullptr; }
void RenderDeviceVulkan::WaitForFence(IFence* fence) {}
IResourceFactory* RenderDeviceVulkan::GetResourceFactory() const { return m_resourceFactory.get(); }
std::unique_ptr<ISwapChain> RenderDeviceVulkan::CreateSwapChain(void* windowHandle, uint32_t width, uint32_t height, bool vsync) { return nullptr; }
ISwapChain* RenderDeviceVulkan::GetSwapChain() const { return m_swapChain.get(); }
IRenderDevice::GPUMemoryInfo RenderDeviceVulkan::GetGPUMemoryInfo() const { return {}; }
IRenderDevice::RenderStats RenderDeviceVulkan::GetRenderStats() const { return m_stats; }
void RenderDeviceVulkan::BeginDebugMarker(const std::string& name) {}
void RenderDeviceVulkan::EndDebugMarker() {}
void RenderDeviceVulkan::SetDebugMarker(const std::string& name) {}
std::string RenderDeviceVulkan::GetName() const { return "Vulkan Device"; }
std::string RenderDeviceVulkan::GetAPIName() const { return "Vulkan"; }
VkRenderPass RenderDeviceVulkan::GetImGuiRenderPass() const { return m_swapChain ? m_swapChain->GetRenderPass() : VK_NULL_HANDLE; }

} // namespace Prisma::Graphic::Vulkan
