#include "RenderDeviceVulkan.h"
#include "VulkanResourceFactory.h"
#include "VulkanSwapChain.h"
#include "Logger.h"
#include <iostream>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#if defined(PRISMA_ENABLE_IMGUI_DEBUG) || defined(PRISMA_BUILD_EDITOR)
#include <imgui_impl_vulkan.h>
#endif

namespace PrismaEngine::Graphic::Vulkan {

RenderDeviceVulkan::RenderDeviceVulkan() {
    m_resourceFactory = std::make_unique<VulkanResourceFactory>(this);
    m_swapChain = std::make_unique<VulkanSwapChain>(this);
}

RenderDeviceVulkan::~RenderDeviceVulkan() {
    Shutdown();
}

bool RenderDeviceVulkan::Initialize(const DeviceDesc& desc) {
    m_desc = desc;
    LOG_INFO("Vulkan", "正在初始化 Vulkan 设备 (vk-bootstrap + VMA)");

    // 1. 创建实例
    LOG_INFO("Vulkan", "正在创建 Vulkan 实例 (Validation: {0})...", desc.enableValidation ? "ON" : "OFF");
    vkb::InstanceBuilder inst_builder;
    auto inst_ret = inst_builder.set_app_name(desc.name.c_str())
                        .request_validation_layers(desc.enableValidation)
                        .use_default_debug_messenger()
                        .require_api_version(1, 1, 0)
                        .build();
    
    // 如果启用验证层失败，尝试在关闭验证层的情况下重新创建
    if (!inst_ret && desc.enableValidation) {
        LOG_WARNING("Vulkan", "启用验证层失败，尝试在无验证层模式下启动...");
        inst_ret = inst_builder.request_validation_layers(false).build();
    }

    if (!inst_ret) {
        LOG_ERROR("Vulkan", "无法创建实例: {0}", inst_ret.error().message());
        return false;
    }
    m_vkbInstance = inst_ret.value();
    m_instance = m_vkbInstance.instance;
    Logger::GetInstance().Flush();

    // 2. 选择物理设备
    LOG_INFO("Vulkan", "正在选择物理设备...");
    vkb::PhysicalDeviceSelector selector{ m_vkbInstance };
    auto phys_ret = selector.set_minimum_version(1, 1)
                        .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
                        .select();
    if (!phys_ret) {
        LOG_ERROR("Vulkan", "无法选择物理设备: {0}", phys_ret.error().message());
        return false;
    }
    m_vkbPhysicalDevice = phys_ret.value();
    m_physicalDevice = m_vkbPhysicalDevice.physical_device;
    Logger::GetInstance().Flush();

    // 3. 创建逻辑设备
    LOG_INFO("Vulkan", "正在创建逻辑设备...");
    vkb::DeviceBuilder device_builder{ m_vkbPhysicalDevice };
    auto dev_ret = device_builder.build();
    if (!dev_ret) {
        LOG_ERROR("Vulkan", "无法创建逻辑设备: {0}", dev_ret.error().message());
        return false;
    }
    m_vkbDevice = dev_ret.value();
    m_device = m_vkbDevice.device;

    // 4. 获取队列
    LOG_INFO("Vulkan", "正在获取队列...");
    auto g_queue_ret = m_vkbDevice.get_queue(vkb::QueueType::graphics);
    if (!g_queue_ret) {
        LOG_ERROR("Vulkan", "无法获取图形队列");
        return false;
    }
    m_graphicsQueue = g_queue_ret.value();
    m_graphicsQueueFamily = m_vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    // 5. 初始化 VMA 分配器
    LOG_INFO("Vulkan", "正在初始化 VMA...");
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_1;
    allocatorInfo.physicalDevice = m_physicalDevice;
    allocatorInfo.device = m_device;
    allocatorInfo.instance = m_instance;
    
    if (vmaCreateAllocator(&allocatorInfo, &m_allocator) != VK_SUCCESS) {
        LOG_ERROR("Vulkan", "无法创建 VMA 分配器");
        return false;
    }

    m_initialized = true;
    LOG_INFO("Vulkan", "Vulkan 设备初始化成功");
    return true;
}

void RenderDeviceVulkan::Shutdown() {
    if (!m_initialized) return;

#if defined(PRISMA_ENABLE_IMGUI_DEBUG) || defined(PRISMA_BUILD_EDITOR)
    if (m_imguiDescriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(m_device, m_imguiDescriptorPool, nullptr);
        m_imguiDescriptorPool = VK_NULL_HANDLE;
    }
#endif

    if (m_allocator != VK_NULL_HANDLE) {
        vmaDestroyAllocator(m_allocator);
        m_allocator = VK_NULL_HANDLE;
    }

    vkb::destroy_device(m_vkbDevice);
    vkb::destroy_instance(m_vkbInstance);

    m_initialized = false;
}

bool RenderDeviceVulkan::InitializeImGui() {
#if defined(PRISMA_ENABLE_IMGUI_DEBUG) || defined(PRISMA_BUILD_EDITOR)
    // 创建 ImGui 描述符池
    VkDescriptorPoolSize pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
    pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;

    if (vkCreateDescriptorPool(m_device, &pool_info, nullptr, &m_imguiDescriptorPool) != VK_SUCCESS) {
        return false;
    }

    // 初始化 ImGui Vulkan 后端
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = m_instance;
    init_info.PhysicalDevice = m_physicalDevice;
    init_info.Device = m_device;
    init_info.QueueFamily = m_graphicsQueueFamily;
    init_info.Queue = m_graphicsQueue;
    init_info.DescriptorPool = m_imguiDescriptorPool;
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;
    
    // 从交换链获取 RenderPass
    if (m_swapChain) {
        init_info.RenderPass = m_swapChain->GetRenderPass();
    }

    // 如果交换链还未准备好 RenderPass（例如初始化早期），ImGui 初始化会失败
    // 在这里我们必须确保提供一个有效的 RenderPass
    if (init_info.RenderPass == VK_NULL_HANDLE) {
        LOG_ERROR("Vulkan", "ImGui requires a valid RenderPass for initialization.");
        return false;
    }
    
    return ImGui_ImplVulkan_Init(&init_info);
#else
    return true;
#endif
}

void RenderDeviceVulkan::ShutdownImGui() {
#if defined(PRISMA_ENABLE_IMGUI_DEBUG) || defined(PRISMA_BUILD_EDITOR)
    ImGui_ImplVulkan_Shutdown();
#endif
}

std::string RenderDeviceVulkan::GetName() const { return "Vulkan Device"; }
std::string RenderDeviceVulkan::GetAPIName() const { return "Vulkan"; }

void RenderDeviceVulkan::WaitForIdle() { vkDeviceWaitIdle(m_device); }

// ... 其余方法暂时返回空或默认实现以保证编译通过 ...
std::unique_ptr<ICommandBuffer> RenderDeviceVulkan::CreateCommandBuffer(CommandBufferType type) { return nullptr; }
void RenderDeviceVulkan::SubmitCommandBuffer(ICommandBuffer* cmdBuffer, IFence* fence) {}
void RenderDeviceVulkan::SubmitCommandBuffers(const std::vector<ICommandBuffer*>& cmdBuffers, const std::vector<IFence*>& fences) {}
std::unique_ptr<IFence> RenderDeviceVulkan::CreateFence() { return nullptr; }
void RenderDeviceVulkan::WaitForFence(IFence* fence) {}
IResourceFactory* RenderDeviceVulkan::GetResourceFactory() const { return nullptr; }
std::unique_ptr<ISwapChain> RenderDeviceVulkan::CreateSwapChain(void* windowHandle, uint32_t width, uint32_t height, bool vsync) { return nullptr; }
ISwapChain* RenderDeviceVulkan::GetSwapChain() const { return nullptr; }
void RenderDeviceVulkan::BeginFrame() {}
void RenderDeviceVulkan::EndFrame() {}
void RenderDeviceVulkan::Present() {}
IRenderDevice::GPUMemoryInfo RenderDeviceVulkan::GetGPUMemoryInfo() const { return {}; }
IRenderDevice::RenderStats RenderDeviceVulkan::GetRenderStats() const { return m_stats; }
void RenderDeviceVulkan::BeginDebugMarker(const std::string& name) {}
void RenderDeviceVulkan::EndDebugMarker() {}
void RenderDeviceVulkan::SetDebugMarker(const std::string& name) {}

} // namespace PrismaEngine::Graphic::Vulkan