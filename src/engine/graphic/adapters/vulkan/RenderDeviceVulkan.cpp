#include "RenderDeviceVulkan.h"
#include "Logger.h"
#include "VulkanResourceFactory.h"
#include "VulkanSwapChain.h"
#include <iostream>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#if defined(PRISMA_ENABLE_IMGUI_DEBUG) || defined(PRISMA_BUILD_EDITOR)
#include <imgui_impl_vulkan.h>
#endif

namespace Prisma::Graphic::Vulkan {

RenderDeviceVulkan::RenderDeviceVulkan() {
    m_resourceFactory = std::make_unique<VulkanResourceFactory>(this);
    m_swapChain       = std::make_unique<VulkanSwapChain>(this);
}

RenderDeviceVulkan::~RenderDeviceVulkan() {
    Shutdown();
}

bool RenderDeviceVulkan::Initialize(const DeviceDesc& desc) {
    m_desc = desc;
    LOG_INFO("Vulkan", "正在初始化 Vulkan 设备 (vk-bootstrap + VMA)");

    // 1. 创建实例
    vkb::InstanceBuilder inst_builder;
    auto inst_ret = inst_builder.set_app_name(desc.name.c_str())
                        .request_validation_layers(desc.enableValidation)
                        .use_default_debug_messenger()
                        .require_api_version(1, 3, 0)
                        .build();

    if (!inst_ret) return false;
    m_vkbInstance = inst_ret.value();
    m_instance    = m_vkbInstance.instance;

    // 2. 选择物理设备
    vkb::PhysicalDeviceSelector selector{m_vkbInstance};
    auto phys_ret = selector.set_minimum_version(1, 3)
                        .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
                        .select();
    if (!phys_ret) return false;
    m_vkbPhysicalDevice = phys_ret.value();
    m_physicalDevice    = m_vkbPhysicalDevice.physical_device;

    // 3. 创建逻辑设备
    vkb::DeviceBuilder device_builder{m_vkbPhysicalDevice};
    auto dev_ret = device_builder.build();
    if (!dev_ret) return false;
    m_vkbDevice = dev_ret.value();
    m_device    = m_vkbDevice.device;

    // 4. 获取队列
    auto g_queue_ret = m_vkbDevice.get_queue(vkb::QueueType::graphics);
    if (!g_queue_ret) return false;
    m_graphicsQueue       = g_queue_ret.value();
    m_graphicsQueueFamily = m_vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    // 5. 初始化 VMA 分配器
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.vulkanApiVersion       = VK_API_VERSION_1_3;
    allocatorInfo.physicalDevice         = m_physicalDevice;
    allocatorInfo.device                 = m_device;
    allocatorInfo.instance               = m_instance;

    if (vmaCreateAllocator(&allocatorInfo, &m_allocator) != VK_SUCCESS) return false;

    m_initialized = true;
    return true;
}

void RenderDeviceVulkan::Shutdown() {
    if (!m_initialized) return;
    
    if (m_allocator != VK_NULL_HANDLE) {
        vmaDestroyAllocator(m_allocator);
        m_allocator = VK_NULL_HANDLE;
    }

    vkb::destroy_device(m_vkbDevice);
    vkb::destroy_instance(m_vkbInstance);
    m_initialized = false;
}

std::unique_ptr<ICommandBuffer> RenderDeviceVulkan::CreateCommandBuffer(CommandBufferType type) { return nullptr; }
void RenderDeviceVulkan::SubmitCommandBuffer(ICommandBuffer* cmdBuffer, IFence* fence) {}
void RenderDeviceVulkan::SubmitCommandBuffers(const std::vector<ICommandBuffer*>& cmdBuffers, const std::vector<IFence*>& fences) {}

void RenderDeviceVulkan::WaitForIdle() {
    if (m_device != VK_NULL_HANDLE) vkDeviceWaitIdle(m_device);
}

std::unique_ptr<IFence> RenderDeviceVulkan::CreateFence() { return nullptr; }
void RenderDeviceVulkan::WaitForFence(IFence* fence) {}

IResourceFactory* RenderDeviceVulkan::GetResourceFactory() const {
    return m_resourceFactory.get();
}

std::unique_ptr<ISwapChain> RenderDeviceVulkan::CreateSwapChain(void* windowHandle, uint32_t width, uint32_t height, bool vsync) {
    return nullptr;
}

ISwapChain* RenderDeviceVulkan::GetSwapChain() const {
    return m_swapChain.get();
}

void RenderDeviceVulkan::BeginFrame() {}
void RenderDeviceVulkan::EndFrame() {}

void RenderDeviceVulkan::Present() {
    if (m_swapChain) m_swapChain->Present();
}

void RenderDeviceVulkan::Resize(uint32_t width, uint32_t height) {
    if (m_swapChain) m_swapChain->Resize(width, height);
}

IRenderDevice::GPUMemoryInfo RenderDeviceVulkan::GetGPUMemoryInfo() const { return {}; }
IRenderDevice::RenderStats RenderDeviceVulkan::GetRenderStats() const { return m_stats; }

bool RenderDeviceVulkan::InitializeImGui() { return true; }
void RenderDeviceVulkan::ShutdownImGui() {}

void RenderDeviceVulkan::BeginDebugMarker(const std::string& name) {}
void RenderDeviceVulkan::EndDebugMarker() {}
void RenderDeviceVulkan::SetDebugMarker(const std::string& name) {}

std::string RenderDeviceVulkan::GetName() const { return "Vulkan Device"; }
std::string RenderDeviceVulkan::GetAPIName() const { return "Vulkan"; }

VkRenderPass RenderDeviceVulkan::GetImGuiRenderPass() const {
    return m_swapChain ? m_swapChain->GetRenderPass() : VK_NULL_HANDLE;
}

} // namespace Prisma::Graphic::Vulkan
