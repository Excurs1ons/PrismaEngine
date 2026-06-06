// [修复] 不在此包含 imgui_impl_vulkan.h
// 目的：Engine.dll 不再直接使用 ImGui Vulkan 后端 API，
//         渲染通过由 PrismaEditor.dll 注册的回调执行，
//         避免两份独立的编译单元共享状态导致指针崩溃。
#include "RenderDeviceVulkan.h"
#include "VulkanCommandBuffer.h"
#include "logger/Logger.h"
#include "VulkanFence.h"
#include "VulkanResourceFactory.h"
#include "VulkanSwapChain.h"
#include <iostream>

// Win32 Vulkan external memory extension names are defined in <vulkan/vulkan_win32.h>,
// which requires <windows.h> (HANDLE type). Define them inline to avoid that dependency.
#if defined(_WIN32) && !defined(VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME)
#define VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME "VK_KHR_external_memory_win32"
#endif
#if defined(_WIN32) && !defined(VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME)
#define VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME "VK_KHR_external_semaphore_win32"
#endif

#define VMA_IMPLEMENTATION
#include <SDL3/SDL_vulkan.h>
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4324) // structure was padded due to alignment specifier
#pragma warning(disable: 4505) // unreferenced local function has been removed
#endif
#include <vk_mem_alloc.h>
#if defined(_MSC_VER)
#pragma warning(pop)
#endif


#include "VulkanResources.h"
#include "app/Engine.h"

namespace Prisma::Graphic::Vulkan {

RenderDeviceVulkan::RenderDeviceVulkan() {
    LOG_INFO("Vulkan", "创建 Vulkan 渲染设备实例");
    m_resourceFactory = std::make_unique<VulkanResourceFactory>(this);
    m_swapChain       = std::make_unique<VulkanSwapChain>(this);
    LOG_DEBUG("Vulkan", "Vulkan 渲染设备实例创建成功");
}

RenderDeviceVulkan::~RenderDeviceVulkan() {
    Shutdown();
}

int RenderDeviceVulkan::Initialize(const DeviceDesc& desc) {
    m_desc = desc;
    m_headless = desc.headless;
    LOG_INFO("Vulkan", "正在初始化 Vulkan 设备{0}", m_headless ? " (headless)" : "");

    try {
        // 1. 创建实例
        vkb::InstanceBuilder inst_builder;
        auto inst_builder_ref = inst_builder.set_app_name(desc.name.c_str())
                                    .request_validation_layers(desc.enableValidation)
                                    .use_default_debug_messenger()
                                    .require_api_version(1, 3, 0)
                                    .set_headless(m_headless);
        if (desc.enableValidation) {
            inst_builder_ref
                .add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT)
                .add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT);
        }
        auto inst_ret = inst_builder_ref.build();

        if (!inst_ret)
            return -1;
        m_vkbInstance = inst_ret.value();
        m_instance    = m_vkbInstance.instance;

        if (!m_headless) {
            auto& window          = Engine::Get().GetWindow();
            SDL_Window* sdlWindow = static_cast<SDL_Window*>(window.GetNativeWindow());
            if (!SDL_Vulkan_CreateSurface(sdlWindow, m_instance, nullptr, &m_surface)) {
                return -1;
            }
        }

// 2. 选择物理设备
        VkPhysicalDeviceFeatures features{};
        features.samplerAnisotropy = VK_TRUE;

        VkPhysicalDeviceVulkan12Features vk12features{};
        vk12features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        vk12features.bufferDeviceAddress = VK_TRUE; // VUID 03331
        vk12features.hostQueryReset = VK_TRUE;      // VUID 02665

        vkb::PhysicalDeviceSelector selector{m_vkbInstance};
        if (!m_headless) {
            selector.set_surface(m_surface);
        }

        // 光线追踪扩展仅在 PathTracing 模式下为必需
        const bool needRayTracing = m_desc.rtMode != RTMode::None;

        // 注意: VK_KHR_dedicated_allocation, VK_KHR_external_memory, VK_KHR_external_semaphore
        // 已晋升到 Vulkan 1.1 核心，在 Vulkan 1.3 下不需要显式请求
        auto selectorBuilder = selector.set_minimum_version(1, 3)
                                   .set_required_features(features)
#ifdef _WIN32
                                   .add_required_extension(VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME)
                                   .add_required_extension(VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME)
#endif
                                   .add_required_extension_features(vk12features)
                                   .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete);

        if (needRayTracing) {
            // Acceleration structure is required for both RayQuery and HardwareRT
            VkPhysicalDeviceAccelerationStructureFeaturesKHR accelFeatures = {};
            accelFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
            accelFeatures.accelerationStructure = VK_TRUE;

            accelFeatures.pNext = &vk12features;

            selectorBuilder = selectorBuilder
                .add_required_extension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME)
                .add_required_extension_features(accelFeatures);
        }

        if (m_desc.rtMode == RTMode::HardwareRT) {
            VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeatures = {};
            rtPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
            rtPipelineFeatures.rayTracingPipeline = VK_TRUE;

            selectorBuilder = selectorBuilder
                .add_required_extension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME)
                .add_required_extension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME)
                .add_required_extension_features(rtPipelineFeatures);
        }

        if (m_desc.rtMode == RTMode::RayQuery) {
            selectorBuilder = selectorBuilder
                .add_required_extension(VK_KHR_RAY_QUERY_EXTENSION_NAME);
        }

        auto phys_ret = selectorBuilder.select();
        if (!phys_ret) {
            if (needRayTracing) {
                LOG_WARN("Vulkan", "物理设备不支持光线追踪扩展，回退到 Forward 渲染模式");
                // 回退：不要求光线追踪扩展，重新选择物理设备
                auto phys_ret_fallback = selector.set_minimum_version(1, 3)
                    .set_required_features(features)
#ifdef _WIN32
                    .add_required_extension(VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME)
                    .add_required_extension(VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME)
#endif
                    .add_required_extension_features(vk12features)
                    .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
                    .select();
                if (!phys_ret_fallback) {
                    LOG_ERROR("Vulkan", "物理设备选择失败（回退模式）: {0}", phys_ret_fallback.error().message());
                    return -2;
                }
                phys_ret = phys_ret_fallback;
                m_desc.rtMode = RTMode::None;
                m_rayTracingSupported = false;
                m_deviceFeatures.supportsRayQuery = false;
                LOG_INFO("Vulkan", "已回退到 Forward 渲染模式");
            } else {
                LOG_ERROR("Vulkan", "物理设备选择失败: {0}", phys_ret.error().message());
                return -2;
            }
        } else {
            m_rayTracingSupported = needRayTracing;
            m_deviceFeatures.supportsRayQuery = (m_desc.rtMode == RTMode::RayQuery && needRayTracing);
        }
        m_vkbPhysicalDevice = phys_ret.value();
        m_physicalDevice    = m_vkbPhysicalDevice.physical_device;

        // 读取 GPU 名称
        {
            VkPhysicalDeviceProperties props{};
            vkGetPhysicalDeviceProperties(m_physicalDevice, &props);
            m_gpuName = props.deviceName;
            LOG_INFO("Vulkan", "GPU: {0} (driver {1}.{2}.{3})",
                props.deviceName,
                VK_VERSION_MAJOR(props.driverVersion),
                VK_VERSION_MINOR(props.driverVersion),
                VK_VERSION_PATCH(props.driverVersion));
        }

        // 3. 创建逻辑设备
        vkb::DeviceBuilder device_builder{m_vkbPhysicalDevice};
        auto dev_ret = device_builder.build();
        if (!dev_ret)
            return -3;
        m_vkbDevice = dev_ret.value();
        m_device    = m_vkbDevice.device;

        // 激活 command buffer 调试标签 (VK_EXT_debug_utils) — 仅在验证模式启用
        if (desc.enableValidation) {
            VulkanCommandBuffer::InitDebugUtils(m_device);
        }

        // 4. 获取队列
        m_graphicsQueue       = m_vkbDevice.get_queue(vkb::QueueType::graphics).value();
        m_graphicsQueueFamily = m_vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

        // 4b. 获取异步计算队列（优先专用的非图形计算队列，否则共享 graphics+compute）
        {
            auto dedicatedCompute = m_vkbDevice.get_dedicated_queue(vkb::QueueType::compute);
            if (dedicatedCompute.has_value()) {
                m_computeQueue = dedicatedCompute.value();
                m_computeQueueFamily = m_vkbDevice.get_dedicated_queue_index(vkb::QueueType::compute).value();
            } else {
                auto computeQueue = m_vkbDevice.get_queue(vkb::QueueType::compute);
                if (computeQueue.has_value()) {
                    m_computeQueue = computeQueue.value();
                    m_computeQueueFamily = m_vkbDevice.get_queue_index(vkb::QueueType::compute).value();
                } else {
                    // 回退：与图形队列共享（总是包含 VK_QUEUE_COMPUTE_BIT）
                    m_computeQueue = m_graphicsQueue;
                    m_computeQueueFamily = m_graphicsQueueFamily;
                }
            }
        }

        // 5. 初始化 VMA（光线追踪需要 VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT）
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.vulkanApiVersion       = VK_API_VERSION_1_3;
        allocatorInfo.physicalDevice         = m_physicalDevice;
        allocatorInfo.device                 = m_device;
        allocatorInfo.instance               = m_instance;
        allocatorInfo.flags                  = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        if (vmaCreateAllocator(&allocatorInfo, &m_allocator) != VK_SUCCESS)
            return -5;

        // 5.1 同步资源工厂的分销器
        if (m_resourceFactory) {
            m_resourceFactory->Initialize(this);
        }



        // 6. 初始化描述符池（包含所有引擎使用的描述符类型）
        std::vector<VkDescriptorPoolSize> poolSizes{
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100},
        };
        if (needRayTracing) {
            poolSizes.push_back({VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 16});
        }

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = 1000;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool);

        // 7. Command Pool & Buffers
        VkCommandPoolCreateInfo cmd_pool_info = {};
        cmd_pool_info.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmd_pool_info.queueFamilyIndex        = m_graphicsQueueFamily;
        cmd_pool_info.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        vkCreateCommandPool(m_device, &cmd_pool_info, nullptr, &m_commandPool);

        // 7b. 计算命令池（如果与图形队列同族则复用同一池，否则创建独立池）
        if (m_computeQueueFamily == m_graphicsQueueFamily) {
            m_computeCommandPool = m_commandPool;
        } else {
            VkCommandPoolCreateInfo cmp_pool_info = {};
            cmp_pool_info.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            cmp_pool_info.queueFamilyIndex        = m_computeQueueFamily;
            cmp_pool_info.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            vkCreateCommandPool(m_device, &cmp_pool_info, nullptr, &m_computeCommandPool);
        }

        m_commandBuffers.resize(3);
        VkCommandBufferAllocateInfo cmd_alloc_info = {};
        cmd_alloc_info.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd_alloc_info.commandPool                 = m_commandPool;
        cmd_alloc_info.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmd_alloc_info.commandBufferCount          = 3;
        vkAllocateCommandBuffers(m_device, &cmd_alloc_info, m_commandBuffers.data());

        // [修复] 包装命令缓冲区
        m_vulkanCommandBuffers.clear();
        for (auto cmd : m_commandBuffers) {
            m_vulkanCommandBuffers.push_back(std::make_unique<VulkanCommandBuffer>(cmd));
        }

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

        // 9. SwapChain (headless模式跳过)
        if (!m_headless) {
            m_swapChain->Initialize(m_surface, desc.width, desc.height, desc.presentMode);

            // 确保 renderFinishedSemaphores 足够大
            uint32_t imageCount = m_swapChain->GetBufferCount();
            if (m_renderFinishedSemaphores.size() < imageCount) {
                size_t oldSize = m_renderFinishedSemaphores.size();
                m_renderFinishedSemaphores.resize(imageCount);
                for (size_t i = oldSize; i < imageCount; i++) {
                    vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]);
                }
            }
        } else {
            // 9b. Headless 模式：创建离屏渲染资源替代交换链
            if (!CreateHeadlessResources(desc.width, desc.height)) {
                LOG_ERROR("Vulkan", "创建离屏渲染资源失败");
                return -7;
            }
        }

        m_deviceFeatures.supportsRayTracing = m_rayTracingSupported;
        m_initialized = true;
        return 0;
    } catch (...) {
        return -999;
    }
}

void RenderDeviceVulkan::Shutdown() {
    if (!m_initialized) {
        LOG_DEBUG("VulkanDevice", "渲染设备无需关闭（未初始化）");
        return;
    }
    
    LOG_DEBUG("VulkanDevice", "正在关闭 Vulkan 渲染设备...");

    // 确保 GPU 已完成所有工作
    if (m_device != VK_NULL_HANDLE) {
        LOG_DEBUG("VulkanDevice", "等待 GPU 空闲...");
        vkDeviceWaitIdle(m_device);
        LOG_DEBUG("VulkanDevice", "GPU 已空闲");
    }

    // 释放所有离屏渲染资源（VkRenderPass / VkFramebuffer），防止引擎退出时泄漏
    VulkanCommandBuffer::ReleaseAllOffscreenResources();

    // 0. 清理 headless 离屏资源（必须在 swapchain 和 VMA 之前）
    DestroyHeadlessResources();

    // 1. 先销毁由此设备管理的子资源
    if (m_resourceFactory) {
        LOG_DEBUG("VulkanDevice", "关闭资源工厂...");
        m_resourceFactory->Shutdown();
        m_resourceFactory.reset();
        LOG_DEBUG("VulkanDevice", "资源工厂已关闭");
    }

    if (m_swapChain) {
        LOG_DEBUG("VulkanDevice", "清理交换链...");
        m_swapChain->Cleanup();
        m_swapChain.reset();
        LOG_DEBUG("VulkanDevice", "交换链已清理");
    }

    // 2. 销毁同步对象和命令池
    LOG_DEBUG("VulkanDevice", "销毁同步对象 (信号量/栅栏)...");
    for (auto s : m_imageAvailableSemaphores)
        vkDestroySemaphore(m_device, s, nullptr);
    for (auto s : m_renderFinishedSemaphores)
        vkDestroySemaphore(m_device, s, nullptr);
    for (auto f : m_inFlightFences)
        vkDestroyFence(m_device, f, nullptr);

    if (m_commandPool) {
        LOG_DEBUG("VulkanDevice", "销毁命令池...");
        vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    }
    if (m_computeCommandPool && m_computeCommandPool != m_commandPool) {
        LOG_DEBUG("VulkanDevice", "销毁计算命令池...");
        vkDestroyCommandPool(m_device, m_computeCommandPool, nullptr);
        m_computeCommandPool = VK_NULL_HANDLE;
    }

    if (m_descriptorPool) {
        LOG_DEBUG("VulkanDevice", "销毁描述符池...");
        vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
        m_descriptorPool = VK_NULL_HANDLE;
    }

    // 3. 销毁基础组件
    if (m_allocator) {
        LOG_DEBUG("VulkanDevice", "销毁 VMA...");
        vmaDestroyAllocator(m_allocator);
        m_allocator = VK_NULL_HANDLE;
    }

    if (m_surface) {
        LOG_DEBUG("Vulkan", "销毁表面...");
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        m_surface = VK_NULL_HANDLE;
    } else {
        LOG_DEBUG("Vulkan", "无 surface 需要销毁（headless模式）");
    }

    // 4. 最后销毁设备和实例
    LOG_DEBUG("VulkanDevice", "销毁 Vulkan 设备和实例...");
    vkb::destroy_device(m_vkbDevice);
    vkb::destroy_instance(m_vkbInstance);
    
    m_device = VK_NULL_HANDLE;
    m_instance = VK_NULL_HANDLE;
    m_initialized = false;

    LOG_DEBUG("VulkanDevice", "Vulkan 渲染设备已完全关闭");
}

void RenderDeviceVulkan::ResetDeviceOnly() {
    if (!m_initialized) {
        LOG_DEBUG("VulkanDevice", "渲染设备无需重置（未初始化）");
        return;
    }
    
    LOG_DEBUG("VulkanDevice", "正在重置 Vulkan 渲染设备（保留实例+Surface）...");

    // 确保 GPU 已完成所有工作
    if (m_device != VK_NULL_HANDLE) {
        LOG_DEBUG("VulkanDevice", "等待 GPU 空闲...");
        vkDeviceWaitIdle(m_device);
        LOG_DEBUG("VulkanDevice", "GPU 已空闲");
    }

    // 释放所有离屏渲染资源
    VulkanCommandBuffer::ReleaseAllOffscreenResources();

    // 清理 headless 离屏资源
    DestroyHeadlessResources();

    // 1. 先销毁由此设备管理的子资源
    if (m_resourceFactory) {
        LOG_DEBUG("VulkanDevice", "关闭资源工厂...");
        m_resourceFactory->Shutdown();
        m_resourceFactory.reset();
        LOG_DEBUG("VulkanDevice", "资源工厂已关闭");
    }

    if (m_swapChain) {
        LOG_DEBUG("VulkanDevice", "清理交换链...");
        m_swapChain->Cleanup();
        m_swapChain.reset();
        LOG_DEBUG("VulkanDevice", "交换链已清理");
    }

    // 2. 销毁同步对象和命令池
    LOG_DEBUG("VulkanDevice", "销毁同步对象 (信号量/栅栏)...");
    for (auto s : m_imageAvailableSemaphores)
        vkDestroySemaphore(m_device, s, nullptr);
    for (auto s : m_renderFinishedSemaphores)
        vkDestroySemaphore(m_device, s, nullptr);
    for (auto f : m_inFlightFences)
        vkDestroyFence(m_device, f, nullptr);

    if (m_commandPool) {
        LOG_DEBUG("VulkanDevice", "销毁命令池...");
        vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    }
    if (m_computeCommandPool && m_computeCommandPool != m_commandPool) {
        LOG_DEBUG("VulkanDevice", "销毁计算命令池...");
        vkDestroyCommandPool(m_device, m_computeCommandPool, nullptr);
        m_computeCommandPool = VK_NULL_HANDLE;
    }

    if (m_descriptorPool) {
        LOG_DEBUG("VulkanDevice", "销毁描述符池...");
        vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
        m_descriptorPool = VK_NULL_HANDLE;
    }

    // 3. 销毁基础组件
    if (m_allocator) {
        LOG_DEBUG("VulkanDevice", "销毁 VMA...");
        vmaDestroyAllocator(m_allocator);
        m_allocator = VK_NULL_HANDLE;
    }

    // 4. 销毁设备（保留实例+Surface）
    LOG_DEBUG("VulkanDevice", "销毁 Vulkan 逻辑设备...");
    vkb::destroy_device(m_vkbDevice);
    
    m_device = VK_NULL_HANDLE;
    m_physicalDevice = VK_NULL_HANDLE;
    m_initialized = false;

    LOG_DEBUG("VulkanDevice", "Vulkan 渲染设备已重置（实例+Surface 保留）");
}

int RenderDeviceVulkan::ReinitializeDevice(const DeviceDesc& desc) {
    m_desc = desc;
    m_headless = desc.headless;
    LOG_INFO("Vulkan", "正在重新初始化 Vulkan 设备{0}", m_headless ? " (headless)" : "");

    try {
        // 跳过: 实例创建 (m_vkbInstance 已存在)
        // 跳过: Surface 创建 (m_surface 已存在)

        // 1. 选择物理设备
        VkPhysicalDeviceFeatures features{};
        features.samplerAnisotropy = VK_TRUE;

        VkPhysicalDeviceVulkan12Features vk12features{};
        vk12features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        vk12features.bufferDeviceAddress = VK_TRUE; // VUID 03331
        vk12features.hostQueryReset = VK_TRUE;      // VUID 02665

        vkb::PhysicalDeviceSelector selector{m_vkbInstance};
        if (!m_headless) {
            selector.set_surface(m_surface);
        }

        // 光线追踪扩展仅在 PathTracing 模式下为必需
        const bool needRayTracing = m_desc.rtMode != RTMode::None;

        // 注意: VK_KHR_dedicated_allocation, VK_KHR_external_memory, VK_KHR_external_semaphore
        // 已晋升到 Vulkan 1.1 核心，在 Vulkan 1.3 下不需要显式请求
        auto selectorBuilder = selector.set_minimum_version(1, 3)
                                   .set_required_features(features)
#ifdef _WIN32
                                   .add_required_extension(VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME)
                                   .add_required_extension(VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME)
#endif
                                   .add_required_extension_features(vk12features)
                                   .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete);

        if (needRayTracing) {
            VkPhysicalDeviceAccelerationStructureFeaturesKHR accelFeatures = {};
            accelFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
            accelFeatures.accelerationStructure = VK_TRUE;

            VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeatures = {};
            rtPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
            rtPipelineFeatures.rayTracingPipeline = VK_TRUE;

            accelFeatures.pNext = &vk12features;

            selectorBuilder = selectorBuilder
                .add_required_extension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME)
                .add_required_extension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME)
                .add_required_extension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME)
                .add_required_extension_features(accelFeatures)
                .add_required_extension_features(rtPipelineFeatures);
        }

        auto phys_ret = selectorBuilder.select();
        if (!phys_ret) {
            if (needRayTracing) {
                LOG_WARN("Vulkan", "物理设备不支持光线追踪扩展，回退到 Forward 渲染模式");
                auto phys_ret_fallback = selector.set_minimum_version(1, 3)
                    .set_required_features(features)
#ifdef _WIN32
                    .add_required_extension(VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME)
                    .add_required_extension(VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME)
#endif
                    .add_required_extension_features(vk12features)
                    .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
                    .select();
                if (!phys_ret_fallback) {
                    LOG_ERROR("Vulkan", "物理设备选择失败（回退模式）: {0}", phys_ret_fallback.error().message());
                    return -2;
                }
                phys_ret = phys_ret_fallback;
                m_desc.rtMode = RTMode::None;
                m_rayTracingSupported = false;
                LOG_INFO("Vulkan", "已回退到 Forward 渲染模式");
            } else {
                LOG_ERROR("Vulkan", "物理设备选择失败: {0}", phys_ret.error().message());
                return -2;
            }
        } else {
            m_rayTracingSupported = needRayTracing;
        }
        m_vkbPhysicalDevice = phys_ret.value();
        m_physicalDevice    = m_vkbPhysicalDevice.physical_device;

        // 读取 GPU 名称
        {
            VkPhysicalDeviceProperties props{};
            vkGetPhysicalDeviceProperties(m_physicalDevice, &props);
            m_gpuName = props.deviceName;
            LOG_INFO("Vulkan", "GPU: {0} (driver {1}.{2}.{3})",
                props.deviceName,
                VK_VERSION_MAJOR(props.driverVersion),
                VK_VERSION_MINOR(props.driverVersion),
                VK_VERSION_PATCH(props.driverVersion));
        }

        // 2. 创建逻辑设备
        vkb::DeviceBuilder device_builder{m_vkbPhysicalDevice};
        auto dev_ret = device_builder.build();
        if (!dev_ret)
            return -3;
        m_vkbDevice = dev_ret.value();
        m_device    = m_vkbDevice.device;

        // 激活 command buffer 调试标签 (VK_EXT_debug_utils) — 仅在验证模式启用
        if (desc.enableValidation) {
            VulkanCommandBuffer::InitDebugUtils(m_device);
        }

        // 3. 获取队列
        m_graphicsQueue       = m_vkbDevice.get_queue(vkb::QueueType::graphics).value();
        m_graphicsQueueFamily = m_vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

        // 3b. 获取异步计算队列
        {
            auto dedicatedCompute = m_vkbDevice.get_dedicated_queue(vkb::QueueType::compute);
            if (dedicatedCompute.has_value()) {
                m_computeQueue = dedicatedCompute.value();
                m_computeQueueFamily = m_vkbDevice.get_dedicated_queue_index(vkb::QueueType::compute).value();
            } else {
                auto computeQueue = m_vkbDevice.get_queue(vkb::QueueType::compute);
                if (computeQueue.has_value()) {
                    m_computeQueue = computeQueue.value();
                    m_computeQueueFamily = m_vkbDevice.get_queue_index(vkb::QueueType::compute).value();
                } else {
                    m_computeQueue = m_graphicsQueue;
                    m_computeQueueFamily = m_graphicsQueueFamily;
                }
            }
        }

        // 4. 初始化 VMA
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.vulkanApiVersion       = VK_API_VERSION_1_3;
        allocatorInfo.physicalDevice         = m_physicalDevice;
        allocatorInfo.device                 = m_device;
        allocatorInfo.instance               = m_instance;
        allocatorInfo.flags                  = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        if (vmaCreateAllocator(&allocatorInfo, &m_allocator) != VK_SUCCESS)
            return -5;

        // 4.1 同步资源工厂的分销器
        if (m_resourceFactory) {
            m_resourceFactory->Initialize(this);
        }

        // 5. 初始化描述符池
        std::vector<VkDescriptorPoolSize> poolSizes{
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100},
        };
        if (needRayTracing) {
            poolSizes.push_back({VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 16});
        }

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = 1000;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool);

        // 6. Command Pool & Buffers
        VkCommandPoolCreateInfo cmd_pool_info = {};
        cmd_pool_info.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmd_pool_info.queueFamilyIndex        = m_graphicsQueueFamily;
        cmd_pool_info.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        vkCreateCommandPool(m_device, &cmd_pool_info, nullptr, &m_commandPool);

        // 6b. 计算命令池
        if (m_computeQueueFamily == m_graphicsQueueFamily) {
            m_computeCommandPool = m_commandPool;
        } else {
            VkCommandPoolCreateInfo cmp_pool_info = {};
            cmp_pool_info.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            cmp_pool_info.queueFamilyIndex        = m_computeQueueFamily;
            cmp_pool_info.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            vkCreateCommandPool(m_device, &cmp_pool_info, nullptr, &m_computeCommandPool);
        }

        m_commandBuffers.resize(3);
        VkCommandBufferAllocateInfo cmd_alloc_info = {};
        cmd_alloc_info.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd_alloc_info.commandPool                 = m_commandPool;
        cmd_alloc_info.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmd_alloc_info.commandBufferCount          = 3;
        vkAllocateCommandBuffers(m_device, &cmd_alloc_info, m_commandBuffers.data());

        m_vulkanCommandBuffers.clear();
        for (auto cmd : m_commandBuffers) {
            m_vulkanCommandBuffers.push_back(std::make_unique<VulkanCommandBuffer>(cmd));
        }

        // 7. Sync Objects
        m_imageAvailableSemaphores.resize(3);
        m_renderFinishedSemaphores.resize(3);
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

        // 8. SwapChain (headless模式跳过)
        if (!m_headless) {
            m_swapChain->Initialize(m_surface, desc.width, desc.height, desc.presentMode);

            uint32_t imageCount = m_swapChain->GetBufferCount();
            if (m_renderFinishedSemaphores.size() < imageCount) {
                size_t oldSize = m_renderFinishedSemaphores.size();
                m_renderFinishedSemaphores.resize(imageCount);
                for (size_t i = oldSize; i < imageCount; i++) {
                    vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]);
                }
            }
        } else {
            // 8b. Headless 模式
            if (!CreateHeadlessResources(desc.width, desc.height)) {
                LOG_ERROR("Vulkan", "创建离屏渲染资源失败");
                return -7;
            }
        }

        m_deviceFeatures.supportsRayTracing = m_rayTracingSupported;
        m_initialized = true;
        return 0;
    } catch (...) {
        return -999;
    }
}

void RenderDeviceVulkan::BeginFrame() {
    if (!m_initialized)
        return;

    m_defaultPassExecuted = false; // 重置本帧标志位

    if (m_headless) {
        vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);
        vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]);

        VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];
        vkResetCommandBuffer(cmd, 0);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(cmd, &beginInfo);

        m_frameActive = true;
        m_isDefaultRenderPassActive = false;
        m_currentFrameIndex = m_currentFrame;
        m_hasPendingPresent = false;
        // 同步重置 VulkanCommandBuffer 的命令计数器（BeginFrame 走原始 Vulkan API，未调用 Begin()）
        if (auto* vkCmdBuf = m_vulkanCommandBuffers[m_currentFrame].get())
            vkCmdBuf->GetAndResetCommandCount();
        return;
    }

    vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

    if (!m_swapChain->AcquireNextImage(m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE)) {
        if (m_swapChain) {
            LOG_WARNING("Vulkan", "AcquireNextImage failed, recreating swapchain");
            vkDeviceWaitIdle(m_device);
            m_swapChain->Resize(m_swapChain->GetWidth(), m_swapChain->GetHeight());
            uint32_t imageCount = m_swapChain->GetBufferCount();
            if (m_renderFinishedSemaphores.size() < imageCount) {
                VkSemaphoreCreateInfo semInfo{};
                semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
                size_t old = m_renderFinishedSemaphores.size();
                m_renderFinishedSemaphores.resize(imageCount);
                for (size_t i = old; i < imageCount; i++)
                    vkCreateSemaphore(m_device, &semInfo, nullptr, &m_renderFinishedSemaphores[i]);
            }
            if (!m_swapChain->AcquireNextImage(m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE))
                return;
        } else {
            return;
        }
    }

    vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]);

    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmd, &beginInfo);

    m_frameActive = true;

    // 同步重置 VulkanCommandBuffer 的命令计数器（BeginFrame 走原始 Vulkan API，未调用 Begin()）
    if (auto* vkCmdBuf = m_vulkanCommandBuffers[m_currentFrame].get())
        vkCmdBuf->GetAndResetCommandCount();

    m_isDefaultRenderPassActive = false;

    m_currentFrameIndex = m_currentFrame;
    m_hasPendingPresent = false;
}

void RenderDeviceVulkan::EndSwapChainRenderPass() {
    if (!m_initialized || !m_frameActive || !m_isDefaultRenderPassActive)
        return;
    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];
    vkCmdEndRenderPass(cmd);
    m_isDefaultRenderPassActive = false;
}

void RenderDeviceVulkan::BeginSwapChainRenderPass(const Prisma::Vector4& clearColorValue) {
    if (!m_initialized || !m_frameActive || m_isDefaultRenderPassActive)
        return;
    m_clearColor = clearColorValue;
    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];
    
    VkRenderPassBeginInfo rpInfo{};
    rpInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.renderArea.offset = {0, 0};

    if (m_headless && m_headlessRenderPass != VK_NULL_HANDLE) {
        rpInfo.renderPass  = m_headlessRenderPass;
        rpInfo.framebuffer = m_headlessFramebuffer;
        rpInfo.renderArea.extent = {m_headlessWidth, m_headlessHeight};
    } else if (!m_headless && m_swapChain) {
        rpInfo.renderPass  = m_swapChain->GetRenderPass();
        rpInfo.framebuffer = m_swapChain->GetCurrentFramebuffer();
        rpInfo.renderArea.extent = m_swapChain->GetExtent();
    } else {
        return;
    }
    
    // 设置两个清除值：0 是颜色，1 是深度
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{m_clearColor.x, m_clearColor.y, m_clearColor.z, m_clearColor.w}};
    clearValues[1].depthStencil = {1.0f, 0};
    
    rpInfo.clearValueCount   = static_cast<uint32_t>(clearValues.size());
    rpInfo.pClearValues      = clearValues.data();
    
    vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
    m_isDefaultRenderPassActive = true;
    m_defaultPassExecuted = true; // 标记本帧已执行过主清屏 Pass
}

void RenderDeviceVulkan::EndFrame() {
    if (!m_initialized || !m_frameActive)
        return;
    VkCommandBuffer cmd = m_commandBuffers[m_currentFrame];

    if (m_headless) {
        // 结束活动中离屏 RenderPass
        if (m_isDefaultRenderPassActive) {
            vkCmdEndRenderPass(cmd);
            m_isDefaultRenderPassActive = false;
        }

        vkEndCommandBuffer(cmd);

        VkSubmitInfo submitInfo{};
        submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = &cmd;

        vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]);
        m_frameActive             = false;
        m_hasPendingPresent       = true;
        return;
    }

    // 如果管线没有手动开启过 SwapChain RP，则在这里开启一个默认的（用于清屏或仅 Overlay）
    if (!m_isDefaultRenderPassActive && !m_defaultPassExecuted) {
        VkRenderPassBeginInfo rpInfo{};
        rpInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpInfo.renderPass        = m_swapChain->GetRenderPass();
        rpInfo.framebuffer       = m_swapChain->GetCurrentFramebuffer();
        rpInfo.renderArea.offset = {0, 0};
        rpInfo.renderArea.extent = m_swapChain->GetExtent();
        
        // 同样设置两个清除值
        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{m_clearColor.x, m_clearColor.y, m_clearColor.z, m_clearColor.w}};
        clearValues[1].depthStencil = {1.0f, 0};
        
        rpInfo.clearValueCount   = static_cast<uint32_t>(clearValues.size());
        rpInfo.pClearValues      = clearValues.data();

        vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
        m_isDefaultRenderPassActive = true;
        m_defaultPassExecuted = true;
    }

    if (m_overlayRenderCallback) {
        // 如果上面没开 RP（因为之前开过又关了），但现在又要画 Overlay，需要以 LOAD 模式重新开启
        // 但目前引擎设计倾向于在同一个 RP 内完成。
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
    m_frameActive = false;
}

void RenderDeviceVulkan::Present() {
    if (!m_initialized || !m_hasPendingPresent)
        return;

    if (m_headless) {
        m_hasPendingPresent = false;
        m_currentFrame = (m_currentFrame + 1) % static_cast<uint32_t>(m_commandBuffers.size());
        return;
    }

    if (!m_swapChain) {
        m_hasPendingPresent = false;
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

        // 调整信号量数组以匹配新的交换链图像数
        uint32_t imageCount = m_swapChain->GetBufferCount();
        if (m_renderFinishedSemaphores.size() < imageCount) {
            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            size_t oldSize = m_renderFinishedSemaphores.size();
            m_renderFinishedSemaphores.resize(imageCount);
            for (size_t i = oldSize; i < imageCount; i++) {
                vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]);
            }
        }
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
RenderDeviceVulkan::CreateSwapChain(void* windowHandle, uint32_t width, uint32_t height, PresentMode presentMode) {
    auto swapChain = std::make_unique<VulkanSwapChain>(this);
    if (swapChain->Initialize(windowHandle, width, height, presentMode) != 0) {
        return nullptr;
    }
    return swapChain;
}
ISwapChain* RenderDeviceVulkan::GetSwapChain() const {
    return m_swapChain.get();
}
IRenderDevice::GPUMemoryInfo RenderDeviceVulkan::GetGPUMemoryInfo() const {
    GPUMemoryInfo info;

#if defined(PRISMA_ENABLE_MEMORY_TRACKING) && PRISMA_ENABLE_MEMORY_TRACKING > 0
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProps);
    for (uint32_t i = 0; i < memProps.memoryHeapCount; ++i) {
        if (memProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
            info.totalMemory += memProps.memoryHeaps[i].size;
        }
    }
    VmaStats vmaStats;
    vmaCalculateStats(m_allocator, &vmaStats);
    info.usedMemory = vmaStats.total.usedBytes;
    info.availableMemory = (info.totalMemory > info.usedMemory)
        ? (info.totalMemory - info.usedMemory) : 0;
#endif

    return info;
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
std::string RenderDeviceVulkan::GetGPUName() const {
    return m_gpuName.empty() ? "Unknown GPU" : m_gpuName;
}

bool RenderDeviceVulkan::IsRayQuerySupported() const {
    return m_deviceFeatures.supportsRayQuery;
}

VkRenderPass RenderDeviceVulkan::GetOverlayRenderPass() const {
    if (m_headless) return VK_NULL_HANDLE;
    return m_swapChain ? m_swapChain->GetRenderPass() : VK_NULL_HANDLE;
}

bool RenderDeviceVulkan::ReadbackImage(VkImage image, uint32_t width, uint32_t height,
                                        VkFormat format, void* outBuffer, size_t bufferSize) {
    if (!m_device || !image || !outBuffer) return false;

    // 根据 format 计算像素字节数 (默认 RGBA32F)
    uint32_t pixelSize = 4 * sizeof(float); // RGBA32F
    switch (format) {
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_UNORM:     pixelSize = 4; break;
        case VK_FORMAT_R8G8B8A8_SRGB:      pixelSize = 4; break;
        case VK_FORMAT_R32G32B32A32_SFLOAT: pixelSize = 16; break;
        case VK_FORMAT_R16G16B16A16_SFLOAT: pixelSize = 8; break;
        case VK_FORMAT_R8_UNORM:            pixelSize = 1; break;
        default: break; // 保持默认 RGBA32F
    }
    VkDeviceSize imageSize = VkDeviceSize(width) * height * pixelSize;

    // 创建 staging buffer
    VkBufferCreateInfo bufCI{};
    bufCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufCI.size = imageSize;
    bufCI.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    VmaAllocationCreateInfo allocCI{};
    allocCI.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
    allocCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT |
                    VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VmaAllocation stagingAlloc = VK_NULL_HANDLE;
    VmaAllocationInfo allocInfo{};
    if (vmaCreateBuffer(m_allocator, &bufCI, &allocCI,
                        &stagingBuffer, &stagingAlloc, &allocInfo) != VK_SUCCESS) {
        return false;
    }

    // 使用临时命令缓冲区执行拷贝
    VkCommandBufferAllocateInfo cmdAI{};
    cmdAI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAI.commandPool = m_commandPool;
    cmdAI.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAI.commandBufferCount = 1;

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(m_device, &cmdAI, &cmd);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    // image 转换到 TRANSFER_SRC_OPTIMAL
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    // 拷贝图像到 staging buffer
    VkBufferImageCopy copyRegion{};
    copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyRegion.imageSubresource.layerCount = 1;
    copyRegion.imageExtent = { width, height, 1 };
    vkCmdCopyImageToBuffer(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           stagingBuffer, 1, &copyRegion);

    // 恢复 image 布局
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    VkFence fence = VK_NULL_HANDLE;
    VkFenceCreateInfo fenceCI{};
    fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    vkCreateFence(m_device, &fenceCI, nullptr, &fence);

    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, fence);
    vkWaitForFences(m_device, 1, &fence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(m_device, fence, nullptr);
    vkFreeCommandBuffers(m_device, m_commandPool, 1, &cmd);

    // 从 staging buffer 拷贝到输出
    void* mapped = allocInfo.pMappedData;
    if (mapped) {
        memcpy(outBuffer, mapped, std::min(imageSize, VkDeviceSize(bufferSize)));
    }

    vmaDestroyBuffer(m_allocator, stagingBuffer, stagingAlloc);
    return true;
}

bool RenderDeviceVulkan::ReadbackTexture(ITexture* texture, uint32_t width, uint32_t height,
                                          void* outBuffer, size_t bufferSize) {
    if (!texture || !outBuffer) return false;
    auto* vkTex = static_cast<VulkanTexture*>(texture);
    if (!vkTex || !vkTex->GetVkImage()) return false;
    return ReadbackImage(vkTex->GetVkImage(), width, height, vkTex->GetVkFormat(), outBuffer, bufferSize);
}

bool RenderDeviceVulkan::CreateHeadlessResources(uint32_t width, uint32_t height) {
    if (!m_device || m_headlessRenderPass != VK_NULL_HANDLE) {
        return false;
    }

    m_headlessWidth  = width;
    m_headlessHeight = height;

    VkDevice device = m_device;
    VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

    // ---------- A. 离屏颜色图像 ----------
    {
        VkImageCreateInfo imgCI{};
        imgCI.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imgCI.imageType     = VK_IMAGE_TYPE_2D;
        imgCI.extent.width  = width;
        imgCI.extent.height = height;
        imgCI.extent.depth  = 1;
        imgCI.mipLevels     = 1;
        imgCI.arrayLayers   = 1;
        imgCI.format        = colorFormat;
        imgCI.tiling        = VK_IMAGE_TILING_OPTIMAL;
        imgCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imgCI.usage         = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                            | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
                            | VK_IMAGE_USAGE_SAMPLED_BIT;
        imgCI.samples       = VK_SAMPLE_COUNT_1_BIT;
        imgCI.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(device, &imgCI, nullptr, &m_headlessColorImage) != VK_SUCCESS) {
            LOG_ERROR("Vulkan", "CreateHeadlessResources: 无法创建离屏颜色图像");
            return false;
        }

        VkMemoryRequirements memReq;
        vkGetImageMemoryRequirements(device, m_headlessColorImage, &memReq);

        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProps);

        uint32_t memType = VK_MAX_MEMORY_TYPES;
        for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
            if ((memReq.memoryTypeBits & (1u << i)) &&
                (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
                memType = i;
                break;
            }
        }
        if (memType == VK_MAX_MEMORY_TYPES) {
            vkDestroyImage(device, m_headlessColorImage, nullptr);
            m_headlessColorImage = VK_NULL_HANDLE;
            return false;
        }

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize  = memReq.size;
        allocInfo.memoryTypeIndex = memType;

        if (vkAllocateMemory(device, &allocInfo, nullptr, &m_headlessColorMemory) != VK_SUCCESS) {
            vkDestroyImage(device, m_headlessColorImage, nullptr);
            m_headlessColorImage  = VK_NULL_HANDLE;
            m_headlessColorMemory = VK_NULL_HANDLE;
            return false;
        }

        vkBindImageMemory(device, m_headlessColorImage, m_headlessColorMemory, 0);

        VkImageViewCreateInfo viewCI{};
        viewCI.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCI.image    = m_headlessColorImage;
        viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewCI.format   = colorFormat;
        viewCI.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        viewCI.subresourceRange.baseMipLevel   = 0;
        viewCI.subresourceRange.levelCount     = 1;
        viewCI.subresourceRange.baseArrayLayer = 0;
        viewCI.subresourceRange.layerCount     = 1;

        if (vkCreateImageView(device, &viewCI, nullptr, &m_headlessColorView) != VK_SUCCESS) {
            return false;
        }
    }

    // ---------- B. 离屏深度图像 ----------
    {
        VkImageCreateInfo imgCI{};
        imgCI.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imgCI.imageType     = VK_IMAGE_TYPE_2D;
        imgCI.extent.width  = width;
        imgCI.extent.height = height;
        imgCI.extent.depth  = 1;
        imgCI.mipLevels     = 1;
        imgCI.arrayLayers   = 1;
        imgCI.format        = depthFormat;
        imgCI.tiling        = VK_IMAGE_TILING_OPTIMAL;
        imgCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imgCI.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imgCI.samples       = VK_SAMPLE_COUNT_1_BIT;
        imgCI.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(device, &imgCI, nullptr, &m_headlessDepthImage) != VK_SUCCESS) {
            LOG_ERROR("Vulkan", "CreateHeadlessResources: 无法创建离屏深度图像");
            return false;
        }

        VkMemoryRequirements memReq;
        vkGetImageMemoryRequirements(device, m_headlessDepthImage, &memReq);

        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProps);

        uint32_t memType = VK_MAX_MEMORY_TYPES;
        for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
            if ((memReq.memoryTypeBits & (1u << i)) &&
                (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
                memType = i;
                break;
            }
        }

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize  = memReq.size;
        allocInfo.memoryTypeIndex = (memType != VK_MAX_MEMORY_TYPES) ? memType : 0u;

        if (vkAllocateMemory(device, &allocInfo, nullptr, &m_headlessDepthMemory) != VK_SUCCESS) {
            return false;
        }

        vkBindImageMemory(device, m_headlessDepthImage, m_headlessDepthMemory, 0);

        VkImageViewCreateInfo viewCI{};
        viewCI.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCI.image    = m_headlessDepthImage;
        viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewCI.format   = depthFormat;
        viewCI.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewCI.subresourceRange.baseMipLevel   = 0;
        viewCI.subresourceRange.levelCount     = 1;
        viewCI.subresourceRange.baseArrayLayer = 0;
        viewCI.subresourceRange.layerCount     = 1;

        if (vkCreateImageView(device, &viewCI, nullptr, &m_headlessDepthView) != VK_SUCCESS) {
            return false;
        }
    }

    // ---------- C. RenderPass ----------
    {
        VkAttachmentDescription colorAtt{};
        colorAtt.format         = colorFormat;
        colorAtt.samples        = VK_SAMPLE_COUNT_1_BIT;
        colorAtt.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAtt.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        colorAtt.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAtt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAtt.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAtt.finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkAttachmentReference colorRef{};
        colorRef.attachment = 0;
        colorRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription depthAtt{};
        depthAtt.format         = depthFormat;
        depthAtt.samples        = VK_SAMPLE_COUNT_1_BIT;
        depthAtt.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAtt.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAtt.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAtt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAtt.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAtt.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthRef{};
        depthRef.attachment = 1;
        depthRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount    = 1;
        subpass.pColorAttachments       = &colorRef;
        subpass.pDepthStencilAttachment = &depthRef;

        VkSubpassDependency dep{};
        dep.srcSubpass    = VK_SUBPASS_EXTERNAL;
        dep.dstSubpass    = 0;
        dep.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
                          | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dep.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
                          | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dep.srcAccessMask = 0;
        dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
                          | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        VkAttachmentDescription attachments[2] = {colorAtt, depthAtt};
        VkRenderPassCreateInfo rpCI{};
        rpCI.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rpCI.attachmentCount = 2;
        rpCI.pAttachments    = attachments;
        rpCI.subpassCount    = 1;
        rpCI.pSubpasses      = &subpass;
        rpCI.dependencyCount = 1;
        rpCI.pDependencies   = &dep;

        if (vkCreateRenderPass(device, &rpCI, nullptr, &m_headlessRenderPass) != VK_SUCCESS) {
            LOG_ERROR("Vulkan", "CreateHeadlessResources: 无法创建离屏 RenderPass");
            return false;
        }
    }

    // ---------- D. Framebuffer ----------
    {
        VkImageView attachments[2] = {m_headlessColorView, m_headlessDepthView};
        VkFramebufferCreateInfo fbCI{};
        fbCI.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbCI.renderPass      = m_headlessRenderPass;
        fbCI.attachmentCount = 2;
        fbCI.pAttachments    = attachments;
        fbCI.width           = width;
        fbCI.height          = height;
        fbCI.layers          = 1;

        if (vkCreateFramebuffer(device, &fbCI, nullptr, &m_headlessFramebuffer) != VK_SUCCESS) {
            LOG_ERROR("Vulkan", "CreateHeadlessResources: 无法创建离屏 Framebuffer");
            return false;
        }
    }

    LOG_INFO("Vulkan", "离屏渲染资源已创建: {}x{} (headless)", width, height);
    return true;
}

void RenderDeviceVulkan::DestroyHeadlessResources() {
    VkDevice device = m_device;
    if (device == VK_NULL_HANDLE)
        return;

    if (m_headlessFramebuffer) {
        vkDestroyFramebuffer(device, m_headlessFramebuffer, nullptr);
        m_headlessFramebuffer = VK_NULL_HANDLE;
    }
    if (m_headlessRenderPass) {
        vkDestroyRenderPass(device, m_headlessRenderPass, nullptr);
        m_headlessRenderPass = VK_NULL_HANDLE;
    }
    if (m_headlessColorView) {
        vkDestroyImageView(device, m_headlessColorView, nullptr);
        m_headlessColorView = VK_NULL_HANDLE;
    }
    if (m_headlessColorImage) {
        vkDestroyImage(device, m_headlessColorImage, nullptr);
        m_headlessColorImage = VK_NULL_HANDLE;
    }
    if (m_headlessColorMemory) {
        vkFreeMemory(device, m_headlessColorMemory, nullptr);
        m_headlessColorMemory = VK_NULL_HANDLE;
    }
    if (m_headlessDepthView) {
        vkDestroyImageView(device, m_headlessDepthView, nullptr);
        m_headlessDepthView = VK_NULL_HANDLE;
    }
    if (m_headlessDepthImage) {
        vkDestroyImage(device, m_headlessDepthImage, nullptr);
        m_headlessDepthImage = VK_NULL_HANDLE;
    }
    if (m_headlessDepthMemory) {
        vkFreeMemory(device, m_headlessDepthMemory, nullptr);
        m_headlessDepthMemory = VK_NULL_HANDLE;
    }

    m_headlessWidth  = 0;
    m_headlessHeight = 0;

    LOG_DEBUG("Vulkan", "离屏渲染资源已销毁");
}

}  // namespace Prisma::Graphic::Vulkan

