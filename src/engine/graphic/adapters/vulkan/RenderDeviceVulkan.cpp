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
        vkb::Result<vkb::Instance> inst_ret = inst_builder.set_app_name(desc.name.c_str())
                            .request_validation_layers(desc.enableValidation)
                            .use_default_debug_messenger()
                            .require_api_version(1, 3, 0)
                            .build();

        if (!inst_ret) {
            LOG_ERROR("Vulkan", "创建 Vulkan 实例失败");
            return -1;
        }
        m_vkbInstance = inst_ret.value();
        m_instance    = m_vkbInstance.instance;

        auto& window          = Engine::Get().GetWindow();
        SDL_Window* sdlWindow = static_cast<SDL_Window*>(window.GetNativeWindow());
        VkSurfaceKHR surface;
        if (!SDL_Vulkan_CreateSurface(sdlWindow, m_instance, nullptr, &surface)) {

            LOG_ERROR("Editor", "Failed to create Vulkan surface for ImGui!");
            return -1;
        }
        // 2. 选择物理设备
        vkb::PhysicalDeviceSelector selector{m_vkbInstance};
        auto phys_ret = selector.set_surface(surface)
            .set_minimum_version(1, 3)
            .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
            .select();
        if (!phys_ret) {
            LOG_ERROR("Vulkan",
                      "选择 Vulkan 物理设备失败: {}",
                      phys_ret.full_error().detailed_failure_reasons.empty()
                          ? "未知错误"
                          : phys_ret.full_error().detailed_failure_reasons[0]);
            return -2;
        }
        m_vkbPhysicalDevice = phys_ret.value();
        m_physicalDevice    = m_vkbPhysicalDevice.physical_device;
        if (!m_vkbPhysicalDevice) {
            LOG_ERROR("Vulkan", "没有找到合适的 Vulkan 物理设备");
            return -2;
        }
        if (!m_physicalDevice) {
            LOG_ERROR("Vulkan", "物理设备句柄无效");
            return -2;
        }
        // 3. 创建逻辑设备
        vkb::DeviceBuilder device_builder{m_vkbPhysicalDevice};
        // 开启动态渲染功能
        //VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering_features = {};
        //dynamic_rendering_features.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
        //dynamic_rendering_features.dynamicRendering = VK_FALSE;  
        //device_builder.add_pNext(&dynamic_rendering_features);
        auto dev_ret = device_builder.build();
        if (!dev_ret) return -3;
        m_vkbDevice = dev_ret.value();
        m_device    = m_vkbDevice.device;

        // 4. 获取队列
        auto g_queue_ret = m_vkbDevice.get_queue(vkb::QueueType::graphics);
        if (!g_queue_ret) return -4;
        m_graphicsQueue       = g_queue_ret.value();
        m_graphicsQueueFamily = m_vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

        // 5. 初始化 VMA 分配器
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.vulkanApiVersion       = VK_API_VERSION_1_3;
        allocatorInfo.physicalDevice         = m_physicalDevice;
        allocatorInfo.device                 = m_device;
        allocatorInfo.instance               = m_instance;

        if (vmaCreateAllocator(&allocatorInfo, &m_allocator) != VK_SUCCESS) return -5;

        m_initialized = true;
        LOG_INFO("Vulkan", "Vulkan 设备初始化成功");
        return 0;
    } catch (const std::exception& e) {
        LOG_ERROR("Vulkan", "创建 Vulkan 实例时发生异常: {}", e.what());
        return -999;
    }

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

void RenderDeviceVulkan::BeginDebugMarker(const std::string& name) {}
void RenderDeviceVulkan::EndDebugMarker() {}
void RenderDeviceVulkan::SetDebugMarker(const std::string& name) {}

std::string RenderDeviceVulkan::GetName() const { return "Vulkan Device"; }
std::string RenderDeviceVulkan::GetAPIName() const { return "Vulkan"; }

VkRenderPass RenderDeviceVulkan::GetImGuiRenderPass() const {
    return m_swapChain ? m_swapChain->GetRenderPass() : VK_NULL_HANDLE;
}

} // namespace Prisma::Graphic::Vulkan
