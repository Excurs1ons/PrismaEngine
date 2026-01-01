#include "RenderAPIVulkan.h"
#include "Logger.h"
#include "LogScope.h"
#include <algorithm>
#include <set>
#include <string>
#include <cstring>

#if defined(__ANDROID__) || defined(ANDROID)
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_android.h>

#endif
namespace PrismaEngine::Graphic {

// ============================================================================
// VulkanRenderDevice - 构造/析构
// ============================================================================

VulkanRenderDevice::VulkanRenderDevice() = default;

VulkanRenderDevice::~VulkanRenderDevice() {
    Shutdown();
}

// ============================================================================
// IRenderDevice 接口实现
// ============================================================================

bool VulkanRenderDevice::Initialize(const DeviceDesc& desc) {
    if (m_initialized) {
        LOG_WARNING("VulkanRenderDevice", "Already initialized");
        return true;
    }

    LOG_INFO("VulkanRenderDevice", "Initializing Vulkan render device...");

    // 1. 创建 Vulkan 实例
    std::vector<const char*> extensions = {
        "VK_KHR_surface",
#ifdef _WIN32
        "VK_KHR_win32_surface",
#elif defined(__ANDROID__)
        "VK_KHR_android_surface",
#else
        "VK_KHR_xlib_surface",
        "VK_KHR_xcb_surface",
        "VK_KHR_wayland_surface",
#endif
    };

    if (!CreateInstance(extensions)) {
        LOG_ERROR("VulkanRenderDevice", "Failed to create Vulkan instance");
        return false;
    }

    // 2. 创建 surface (从 windowHandle)
#ifdef _WIN32
    VkWin32SurfaceCreateInfoKHR surfaceInfo = {};
    surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceInfo.hwnd = static_cast<HWND>(desc.windowHandle);
    surfaceInfo.hinstance = GetModuleHandle(nullptr);

    if (vkCreateWin32SurfaceKHR(m_instance, &surfaceInfo, nullptr, &m_surface) != VK_SUCCESS) {
        LOG_ERROR("VulkanRenderDevice", "Failed to create Win32 surface");
        return false;
    }
#elif defined(__ANDROID__)
    VkAndroidSurfaceCreateInfoKHR surfaceInfo = {};
    surfaceInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    surfaceInfo.window = static_cast<ANativeWindow*>(desc.windowHandle);

    if (vkCreateAndroidSurfaceKHR(m_instance, &surfaceInfo, nullptr, &m_surface) != VK_SUCCESS) {
        LOG_ERROR("VulkanRenderDevice", "Failed to create Android surface");
        return false;
    }
#else
    // SDL3 surface 创建需要特殊处理
    LOG_WARNING("VulkanRenderDevice", "SDL3 surface creation not implemented");
    return false;
#endif

    // 3. 选择物理设备
    if (!PickPhysicalDevice(m_surface)) {
        LOG_ERROR("VulkanRenderDevice", "Failed to pick physical device");
        return false;
    }

    // 4. 创建逻辑设备
    if (!CreateLogicalDevice()) {
        LOG_ERROR("VulkanRenderDevice", "Failed to create logical device");
        return false;
    }

    // 5. 创建交换链
    if (!CreateSwapChain(desc.width, desc.height, desc.vsync)) {
        LOG_ERROR("VulkanRenderDevice", "Failed to create swap chain");
        return false;
    }

    // 6. 创建渲染通道
    if (!CreateRenderPass()) {
        LOG_ERROR("VulkanRenderDevice", "Failed to create render pass");
        return false;
    }

    // 7. 创建帧缓冲
    if (!CreateFramebuffers()) {
        LOG_ERROR("VulkanRenderDevice", "Failed to create framebuffers");
        return false;
    }

    // 8. 创建命令池
    if (!CreateCommandPool()) {
        LOG_ERROR("VulkanRenderDevice", "Failed to create command pool");
        return false;
    }

    // 9. 创建同步对象
    if (!CreateSyncObjects()) {
        LOG_ERROR("VulkanRenderDevice", "Failed to create sync objects");
        return false;
    }

    // 10. 创建资源工厂
    // m_resourceFactory = std::make_unique<VulkanResourceFactory>(this);

    m_initialized = true;
    LOG_INFO("VulkanRenderDevice", "Initialized successfully: {0}", m_deviceName);
    return true;
}

void VulkanRenderDevice::Shutdown() {
    if (!m_initialized) {
        return;
    }

    LOG_INFO("VulkanRenderDevice", "Shutting down...");

    // 等待设备空闲
    if (m_device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(m_device);
    }

    // 清理同步对象
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (m_renderFinishedSemaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
        }
        if (m_imageAvailableSemaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
        }
        if (m_inFlightFences[i] != VK_NULL_HANDLE) {
            vkDestroyFence(m_device, m_inFlightFences[i], nullptr);
        }
    }

    // 清理帧缓冲
    for (auto framebuffer : m_swapChainFramebuffers) {
        if (framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(m_device, framebuffer, nullptr);
        }
    }

    // 清理命令池
    if (m_commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    }

    // 清理渲染通道
    if (m_renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(m_device, m_renderPass, nullptr);
    }

    // 清理交换链
    for (auto imageView : m_swapChainImageViews) {
        if (imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(m_device, imageView, nullptr);
        }
    }

    if (m_swapChain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
    }

    // 清理 surface
    if (m_surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    }

    // 清理逻辑设备
    if (m_device != VK_NULL_HANDLE) {
        vkDestroyDevice(m_device, nullptr);
    }

    // 清理实例
    if (m_instance != VK_NULL_HANDLE) {
        vkDestroyInstance(m_instance, nullptr);
    }

    // 清理资源工厂
    m_resourceFactory.reset();
    m_swapChainObj.reset();

    // 重置状态
    m_instance = VK_NULL_HANDLE;
    m_physicalDevice = VK_NULL_HANDLE;
    m_device = VK_NULL_HANDLE;
    m_surface = VK_NULL_HANDLE;
    m_swapChain = VK_NULL_HANDLE;
    m_renderPass = VK_NULL_HANDLE;
    m_commandPool = VK_NULL_HANDLE;
    m_initialized = false;

    LOG_INFO("VulkanRenderDevice", "Shutdown complete");
}

std::string VulkanRenderDevice::GetName() const {
    return m_deviceName;
}

std::string VulkanRenderDevice::GetAPIName() const {
    return "Vulkan";
}

// ============================================================================
// 命令缓冲区管理
// ============================================================================

std::unique_ptr<ICommandBuffer> VulkanRenderDevice::CreateCommandBuffer(CommandBufferType type) {
    // TODO: 实现命令缓冲区创建
    LOG_WARNING("VulkanRenderDevice", "CreateCommandBuffer not fully implemented");
    return nullptr;
}

void VulkanRenderDevice::SubmitCommandBuffer(ICommandBuffer* cmdBuffer, IFence* fence) {
    // TODO: 实现命令提交
    LOG_WARNING("VulkanRenderDevice", "SubmitCommandBuffer not fully implemented");
}

void VulkanRenderDevice::SubmitCommandBuffers(const std::vector<ICommandBuffer*>& cmdBuffers,
                                              const std::vector<IFence*>& fences) {
    // TODO: 实现批量命令提交
    LOG_WARNING("VulkanRenderDevice", "SubmitCommandBuffers not fully implemented");
}

// ============================================================================
// 同步操作
// ============================================================================

void VulkanRenderDevice::WaitForIdle() {
    if (m_device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(m_device);
    }
}

std::unique_ptr<IFence> VulkanRenderDevice::CreateFence() {
    // TODO: 实现 Fence 创建
    LOG_WARNING("VulkanRenderDevice", "CreateFence not fully implemented");
    return nullptr;
}

void VulkanRenderDevice::WaitForFence(IFence* fence) {
    // TODO: 实现 Fence 等待
    LOG_WARNING("VulkanRenderDevice", "WaitForFence not fully implemented");
}

// ============================================================================
// 资源管理
// ============================================================================

IResourceFactory* VulkanRenderDevice::GetResourceFactory() const {
    return m_resourceFactory.get();
}

// ============================================================================
// 交换链管理
// ============================================================================

std::unique_ptr<ISwapChain> VulkanRenderDevice::CreateSwapChain(void* windowHandle,
                                                                 uint32_t width,
                                                                 uint32_t height,
                                                                 bool vsync) {
    // TODO: 实现交换链创建
    LOG_WARNING("VulkanRenderDevice", "CreateSwapChain not fully implemented");
    return nullptr;
}

ISwapChain* VulkanRenderDevice::GetSwapChain() const {
    return m_swapChainObj.get();
}

// ============================================================================
// 帧管理
// ============================================================================

void VulkanRenderDevice::BeginFrame() {
    if (!m_initialized) {
        return;
    }

    // 等待上一帧完成
    vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrameIndex], VK_TRUE, UINT64_MAX);

    // 获取下一张图像
    VkResult result = vkAcquireNextImageKHR(
        m_device,
        m_swapChain,
        UINT64_MAX,
        m_imageAvailableSemaphores[m_currentFrameIndex],
        VK_NULL_HANDLE,
        &m_currentImageIndex
    );

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        // 交换链需要重建
        LOG_WARNING("VulkanRenderDevice", "Swap chain out of date, needs recreation");
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        LOG_ERROR("VulkanRenderDevice", "Failed to acquire swap chain image");
        return;
    }

    // 重置围栏
    vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrameIndex]);

    m_stats.frameCount++;
}

void VulkanRenderDevice::EndFrame() {
    // 帧结束处理
    LOG_DEBUG("VulkanRenderDevice", "EndFrame");
}

void VulkanRenderDevice::Present() {
    if (!m_initialized) {
        return;
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    VkSemaphore signalSemaphores[] = {m_renderFinishedSemaphores[m_currentFrameIndex]};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {m_swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &m_currentImageIndex;

    VkResult result = vkQueuePresentKHR(m_presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        LOG_WARNING("VulkanRenderDevice", "Swap chain needs recreation after present");
    } else if (result != VK_SUCCESS) {
        LOG_ERROR("VulkanRenderDevice", "Failed to present swap chain image");
    }

    // 移动到下一帧
    m_currentFrameIndex = (m_currentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

// ============================================================================
// 功能查询
// ============================================================================

bool VulkanRenderDevice::SupportsMultiThreaded() const {
    return true; // Vulkan 原生支持多线程
}

bool VulkanRenderDevice::SupportsBindlessTextures() const {
    // TODO: 检查是否支持 VK_EXT_descriptor_indexing
    return false;
}

bool VulkanRenderDevice::SupportsComputeShader() const {
    return true; // Vulkan 必须支持计算着色器
}

bool VulkanRenderDevice::SupportsRayTracing() const {
    // TODO: 检查 VK_KHR_ray_tracing
    return false;
}

bool VulkanRenderDevice::SupportsMeshShader() const {
    // TODO: 检查 VK_EXT_mesh_shader
    return false;
}

bool VulkanRenderDevice::SupportsVariableRateShading() const {
    // TODO: 检查 VK_KHR_fragment_shading_rate
    return false;
}

IRenderDevice::GPUMemoryInfo VulkanRenderDevice::GetGPUMemoryInfo() const {
    GPUMemoryInfo info = {};
    // TODO: 实现 VK_EXT_memory_budget 或 VK_NV_memory_budget
    return info;
}

IRenderDevice::RenderStats VulkanRenderDevice::GetRenderStats() const {
    return m_stats;
}

// ============================================================================
// 调试
// ============================================================================

void VulkanRenderDevice::BeginDebugMarker(const std::string& name) {
    // TODO: 实现 VK_EXT_debug_marker
    LOG_DEBUG("VulkanRenderDevice", "BeginDebugMarker: {0}", name);
}

void VulkanRenderDevice::EndDebugMarker() {
    LOG_DEBUG("VulkanRenderDevice", "EndDebugMarker");
}

void VulkanRenderDevice::SetDebugMarker(const std::string& name) {
    LOG_DEBUG("VulkanRenderDevice", "SetDebugMarker: {0}", name);
}

// ============================================================================
// Vulkan 初始化辅助方法
// ============================================================================

bool VulkanRenderDevice::CreateInstance(const std::vector<const char*>& requiredExtensions) {
    // 获取支持的扩展
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

    LOG_INFO("VulkanRenderDevice", "Available extensions:");
    for (const auto& ext : availableExtensions) {
        LOG_INFO("VulkanRenderDevice", "  - {0}", ext.extensionName);
    }

    // 验证所需扩展
    for (const auto& requiredExt : requiredExtensions) {
        bool found = false;
        for (const auto& availableExt : availableExtensions) {
            if (std::strcmp(requiredExt, availableExt.extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            LOG_ERROR("VulkanRenderDevice", "Required extension not found: {0}", requiredExt);
            return false;
        }
    }

    // 创建应用信息
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Prisma Engine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Prisma Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    // 创建实例信息
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

    // 启用验证层（调试时）
#ifdef _DEBUG
    const char* validationLayers[] = {"VK_LAYER_KHRONOS_validation"};
    createInfo.enabledLayerCount = 1;
    createInfo.ppEnabledLayerNames = validationLayers;
#endif

    // 创建实例
    VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
    if (result != VK_SUCCESS) {
        LOG_ERROR("VulkanRenderDevice", "Failed to create Vulkan instance: {0}", static_cast<int>(result));
        return false;
    }

    LOG_INFO("VulkanRenderDevice", "Vulkan instance created successfully");
    return true;
}

bool VulkanRenderDevice::PickPhysicalDevice(const VkSurfaceKHR surface) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        LOG_ERROR("VulkanRenderDevice", "No physical devices found");
        return false;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

    // 寻找合适的设备
    for (const auto& device : devices) {
        if (IsDeviceSuitable(device, surface)) {
            m_physicalDevice = device;

            // 获取设备名称
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(device, &properties);
            m_deviceName = properties.deviceName;

            LOG_INFO("VulkanRenderDevice", "Selected physical device: {0}", m_deviceName);
            return true;
        }
    }

    LOG_ERROR("VulkanRenderDevice", "No suitable physical device found");
    return false;
}

bool VulkanRenderDevice::CreateLogicalDevice() {
    QueueFamilyIndices indices = FindQueueFamilies(m_physicalDevice, m_surface);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {
        indices.graphicsFamily.value(),
        indices.presentFamily.value()
    };

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // 设备特性
    VkPhysicalDeviceFeatures deviceFeatures = {};

    // 启用所需扩展
    std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    VkResult result = vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device);
    if (result != VK_SUCCESS) {
        LOG_ERROR("VulkanRenderDevice", "Failed to create logical device");
        return false;
    }

    vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);

    m_graphicsQueueFamily = indices.graphicsFamily.value();
    m_presentQueueFamily = indices.presentFamily.value();

    LOG_INFO("VulkanRenderDevice", "Logical device created successfully");
    return true;
}

bool VulkanRenderDevice::CreateSwapChain(uint32_t width, uint32_t height, bool vsync) {
    // 获取交换链支持详情
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, formats.data());

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentModeCount, presentModes.data());

    // 选择格式
    VkSurfaceFormatKHR surfaceFormat = formats[0];
    for (const auto& format : formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = format;
            break;
        }
    }

    // 选择呈现模式
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR; // vsync 默认开启
    if (!vsync) {
        for (const auto& mode : presentModes) {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                presentMode = mode;
                break;
            }
            if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                presentMode = mode;
            }
        }
    }

    // 选择分辨率
    VkExtent2D extent = capabilities.currentExtent;
    if (capabilities.currentExtent.width == UINT32_MAX) {
        extent.width = std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        extent.height = std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }

    // 图像数量
    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    // 创建交换链
    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = FindQueueFamilies(m_physicalDevice, m_surface);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VkResult result = vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain);
    if (result != VK_SUCCESS) {
        LOG_ERROR("VulkanRenderDevice", "Failed to create swap chain");
        return false;
    }

    m_swapChainImageFormat = surfaceFormat.format;
    m_swapChainExtent = extent;

    // 获取交换链图像
    vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, nullptr);
    m_swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, m_swapChainImages.data());

    LOG_INFO("VulkanRenderDevice", "Swap chain created: {0}x{1}, format: {2}",
             extent.width, extent.height, (int)surfaceFormat.format);

    return true;
}

bool VulkanRenderDevice::CreateImageViews() {
    m_swapChainImageViews.resize(m_swapChainImages.size());

    for (size_t i = 0; i < m_swapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_swapChainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VkResult result = vkCreateImageView(m_device, &createInfo, nullptr, &m_swapChainImageViews[i]);
        if (result != VK_SUCCESS) {
            LOG_ERROR("VulkanRenderDevice", "Failed to create image view {0}", i);
            return false;
        }
    }

    LOG_INFO("VulkanRenderDevice", "Created {0} image views", m_swapChainImageViews.size());
    return true;
}

bool VulkanRenderDevice::CreateRenderPass() {
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = m_swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VkResult result = vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass);
    if (result != VK_SUCCESS) {
        LOG_ERROR("VulkanRenderDevice", "Failed to create render pass");
        return false;
    }

    LOG_INFO("VulkanRenderDevice", "Render pass created successfully");
    return true;
}

bool VulkanRenderDevice::CreateFramebuffers() {
    m_swapChainFramebuffers.resize(m_swapChainImageViews.size());

    for (size_t i = 0; i < m_swapChainImageViews.size(); i++) {
        VkImageView attachments[] = {
            m_swapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = m_swapChainExtent.width;
        framebufferInfo.height = m_swapChainExtent.height;
        framebufferInfo.layers = 1;

        VkResult result = vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]);
        if (result != VK_SUCCESS) {
            LOG_ERROR("VulkanRenderDevice", "Failed to create framebuffer {0}", i);
            return false;
        }
    }

    LOG_INFO("VulkanRenderDevice", "Created {0} framebuffers", m_swapChainFramebuffers.size());
    return true;
}

bool VulkanRenderDevice::CreateCommandPool() {
    QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(m_physicalDevice, m_surface);

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    VkResult result = vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool);
    if (result != VK_SUCCESS) {
        LOG_ERROR("VulkanRenderDevice", "Failed to create command pool");
        return false;
    }

    LOG_INFO("VulkanRenderDevice", "Command pool created successfully");
    return true;
}

bool VulkanRenderDevice::CreateSyncObjects() {
    m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    m_imagesInFlight.resize(m_swapChainImages.size(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS) {
            LOG_ERROR("VulkanRenderDevice", "Failed to create sync objects for frame {0}", i);
            return false;
        }
    }

    LOG_INFO("VulkanRenderDevice", "Sync objects created successfully");
    return true;
}

VulkanRenderDevice::QueueFamilyIndices VulkanRenderDevice::FindQueueFamilies(
    VkPhysicalDevice device, VkSurfaceKHR surface) const {

    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.IsComplete()) {
            break;
        }

        i++;
    }

    return indices;
}

bool VulkanRenderDevice::IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) const {
    QueueFamilyIndices indices = FindQueueFamilies(device, surface);
    if (!indices.IsComplete()) {
        return false;
    }

    // 检查扩展支持
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    if (!requiredExtensions.empty()) {
        return false;
    }

    // 检查交换链支持
    bool swapChainAdequate = false;
    if (surface != VK_NULL_HANDLE) {
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
        swapChainAdequate = formatCount != 0 && presentModeCount != 0;
    }

    return swapChainAdequate;
}

uint32_t VulkanRenderDevice::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    LOG_ERROR("VulkanRenderDevice", "Failed to find suitable memory type");
    return UINT32_MAX;
}

} // namespace PrismaEngine::Graphic
