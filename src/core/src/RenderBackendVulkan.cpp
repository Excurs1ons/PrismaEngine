#include "RenderBackendVulkan.h"
#include "LogScope.h"
#include <vector>
#include <stdexcept>
#include "Logger.h"
#include "Camera2D.h"
#include "ApplicationWindows.h"
#include <unordered_map>
#include <mutex>
#include "SceneManager.h"
#include <vulkan/vulkan.h>

namespace Engine {
// 用于在不修改头文件的情况下将每个实例的 acquired imageIndex 保持到 EndFrame
static std::unordered_map<void*, uint32_t> s_acquiredImageIndex;
static std::mutex s_acquiredImageMutex;

RendererVulkan::RendererVulkan()
    : instance(VK_NULL_HANDLE), physicalDevice(VK_NULL_HANDLE), device(VK_NULL_HANDLE), graphicsQueue(VK_NULL_HANDLE),
      renderPass(VK_NULL_HANDLE), swapChain(VK_NULL_HANDLE), commandPool(VK_NULL_HANDLE),
      imageAvailableSemaphore(VK_NULL_HANDLE), renderFinishedSemaphore(VK_NULL_HANDLE), inFlightFence(VK_NULL_HANDLE) {}
bool RendererVulkan::Initialize(Platform* platform, void* windowHandle, void* surface, uint32_t width, uint32_t height) {
    try {
        // 1. 如果 instance 为空，创建它
        if (instance == VK_NULL_HANDLE) {
            if (!platform) {
                throw std::runtime_error("Platform not provided for Vulkan initialization");
            }
            auto extensions = platform->GetVulkanInstanceExtensions();
            if (!CreateInstance(extensions.data(), (uint32_t)extensions.size())) {
                throw std::runtime_error("Failed to create Vulkan instance");
            }
        }

        // 2. 如果 surface 为空，创建它
        if (surface == nullptr) {
             if (!platform) {
                 throw std::runtime_error("Platform not provided for Vulkan surface creation");
             }
             void* createdSurface = nullptr;
             if (!platform->CreateVulkanSurface(instance, windowHandle, &createdSurface)) {
                 throw std::runtime_error("Failed to create Vulkan surface");
             }
             surface = createdSurface;
        }

        // 先选择物理设备并创建逻辑设备
        PickPhysicalDevice();
        CreateLogicalDevice();

        m_windowHandle           = windowHandle;
        m_surface                = (VkSurfaceKHR)(uintptr_t)surface;
        m_swapchainExtent.width  = width;
        m_swapchainExtent.height = height;

        // 现在安全地创建交换链和相关资源
        CreateSwapChain();
        CreateImageViews();
        CreateRenderPass();
        CreateFramebuffers();
        CreateCommandPool();
        CreateCommandBuffers();
        // 创建同步对象
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects!");
        }

        isInitialized = true;
        LOG_INFO("Vulkan", "Vulkan渲染器初始化成功");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Vulkan", "Vulkan渲染器无法初始化: {0}", e.what());
        return false;
    }
}

void RendererVulkan::Shutdown() {
    vkDestroyFence(device, inFlightFence, nullptr);
    vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
    vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);

    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);

    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device, swapChain, nullptr);
    vkDestroyDevice(device, nullptr);

    if (instance != VK_NULL_HANDLE && m_surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance, m_surface, nullptr);
        m_surface = VK_NULL_HANDLE;
    }

    vkDestroyInstance(instance, nullptr);
    isInitialized = false;
    LOG_INFO("Vulkan", "Vulkan renderer shutdown completed");
}

void RendererVulkan::BeginFrame() {
    // 创建渲染帧日志作用域
    LogScope* frameScope = LogScopeManager::GetInstance().CreateScope("VulkanFrame");
    Logger::GetInstance().PushLogScope(frameScope);

    // 等待上一帧完成
    vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &inFlightFence);

    // 获取交换链图像
    uint32_t imageIndex = UINT32_MAX;
    VkResult acquireRes =
        vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    if (acquireRes == VK_ERROR_OUT_OF_DATE_KHR) {
        LOG_WARNING("RendererVulkan",
                    "vkAcquireNextImageKHR returned VK_ERROR_OUT_OF_DATE_KHR - swapchain out of date (BeginFrame)");
        // 上层应处理重建 swapchain，这里安全返回
        Logger::GetInstance().PopLogScope(frameScope);
        LogScopeManager::GetInstance().DestroyScope(frameScope, true);  // 正常退出，不输出日志
        return;
    }
    if (acquireRes != VK_SUCCESS && acquireRes != VK_SUBOPTIMAL_KHR) {
        LOG_ERROR("RendererVulkan", "vkAcquireNextImageKHR failed: {0}", static_cast<int>(acquireRes));
        Logger::GetInstance().PopLogScope(frameScope);
        LogScopeManager::GetInstance().DestroyScope(frameScope, false);  // 出错，输出日志
        return;
    }
    if (imageIndex >= swapChainImages.size()) {
        LOG_ERROR("RendererVulkan",
                  "vkAcquireNextImageKHR returned invalid imageIndex {0} (count={1})",
                  imageIndex,
                  swapChainImages.size());
        Logger::GetInstance().PopLogScope(frameScope);
        LogScopeManager::GetInstance().DestroyScope(frameScope, false);  // 出错，输出日志
        return;
    }

    // 保存 imageIndex 以便 EndFrame 使用（线程安全）
    {
        std::lock_guard<std::mutex> lock(s_acquiredImageMutex);
        s_acquiredImageIndex[this] = imageIndex;
    }

    // 重置命令缓冲区
    vkResetCommandBuffer(commandBuffers[imageIndex], 0);

    // 开始记录命令缓冲区
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;  // 不使用一次性提交标志

    if (vkBeginCommandBuffer(commandBuffers[imageIndex], &beginInfo) != VK_SUCCESS) {
        LOG_ERROR("RendererVulkan", "failed to begin recording command buffer!");
        Logger::GetInstance().PopLogScope(frameScope);
        LogScopeManager::GetInstance().DestroyScope(frameScope, false);  // 出错，输出日志
        return;
    }

    // 开始渲染过程
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = renderPass;
    renderPassInfo.framebuffer       = swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_swapchainExtent;

    // 从场景获取主相机和清除颜色，默认为青色
    XMVECTOR clearColorValue = XMVectorSet(0.0f, 1.0f, 1.0f, 1.0f);  // 默认青色
    auto scene = SceneManager::GetInstance()->GetCurrentScene();
    // 尝试从场景中获取主相机
    Camera* mainCamera = nullptr;
    if (scene) {
        mainCamera = scene->GetMainCamera();
    }
    
    if (mainCamera) {
        // 从主相机获取清除颜色
        clearColorValue = mainCamera->GetClearColor();
        LOG_DEBUG("RendererVulkan",
                    "Using main camera clear color: ({0}, {1}, {2}, {3})",
                    XMVectorGetX(clearColorValue),
                    XMVectorGetY(clearColorValue),
                    XMVectorGetZ(clearColorValue),
                    XMVectorGetW(clearColorValue));
    } else {
        LOG_DEBUG("RendererVulkan", "No main camera found in scene, using default clear color");
    }

    // 转换DirectXMath向量为Vulkan清除颜色
    VkClearValue clearColor = {{{XMVectorGetX(clearColorValue),
                                 XMVectorGetY(clearColorValue),
                                 XMVectorGetZ(clearColorValue),
                                 XMVectorGetW(clearColorValue)}}};

    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues    = &clearColor;

    vkCmdBeginRenderPass(commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // 在这里可以添加绘制命令，但现在只是清屏所以不需要
    // vkCmdDraw(...);

    // vkCmdEndRenderPass 和 vkEndCommandBuffer 移至 EndFrame
    // 正常结束BeginFrame，继续保持作用域活跃到EndFrame
}

void RendererVulkan::EndFrame() {
    // 获取当前日志作用域
    LogScope* frameScope = Logger::GetInstance().GetCurrentLogScope();

    // 1) 基本有效性检查
    if (device == VK_NULL_HANDLE) {
        LOG_ERROR("RendererVulkan", "EndFrame: device is VK_NULL_HANDLE");
        if (frameScope) {
            Logger::GetInstance().PopLogScope(frameScope);
            LogScopeManager::GetInstance().DestroyScope(frameScope, false);  // 出错，输出日志
        }
        return;
    }
    if (swapChain == VK_NULL_HANDLE) {
        LOG_ERROR("RendererVulkan", "EndFrame: swapChain is VK_NULL_HANDLE");
        if (frameScope) {
            Logger::GetInstance().PopLogScope(frameScope);
            LogScopeManager::GetInstance().DestroyScope(frameScope, false);  // 出错，输出日志
        }
        return;
    }
    if (graphicsQueue == VK_NULL_HANDLE) {
        LOG_ERROR("RendererVulkan", "EndFrame: graphicsQueue is VK_NULL_HANDLE");
        if (frameScope) {
            Logger::GetInstance().PopLogScope(frameScope);
            LogScopeManager::GetInstance().DestroyScope(frameScope, false);  // 出错，输出日志
        }
        return;
    }
    if (imageAvailableSemaphore == VK_NULL_HANDLE) {
        LOG_WARNING("RendererVulkan", "EndFrame: imageAvailableSemaphore is VK_NULL_HANDLE");
    }
    if (renderFinishedSemaphore == VK_NULL_HANDLE) {
        LOG_WARNING("RendererVulkan", "EndFrame: renderFinishedSemaphore is VK_NULL_HANDLE");
    }
    if (commandBuffers.empty()) {
        LOG_ERROR("RendererVulkan", "EndFrame: no command buffers allocated");
        if (frameScope) {
            Logger::GetInstance().PopLogScope(frameScope);
            LogScopeManager::GetInstance().DestroyScope(frameScope, false);  // 出错，输出日志
        }
        return;
    }
    if (swapChainImages.empty()) {
        LOG_ERROR("RendererVulkan", "EndFrame: no swap chain images");
        if (frameScope) {
            Logger::GetInstance().PopLogScope(frameScope);
            LogScopeManager::GetInstance().DestroyScope(frameScope, false);  // 出错，输出日志
        }
        return;
    }

    // 从 BeginFrame 保存的位置获取 imageIndex —— 不要在 EndFrame 再次调用 vkAcquireNextImageKHR
    uint32_t imageIndex = UINT32_MAX;
    {
        std::lock_guard<std::mutex> lock(s_acquiredImageMutex);
        auto it = s_acquiredImageIndex.find(this);
        if (it == s_acquiredImageIndex.end()) {
            LOG_ERROR("RendererVulkan",
                      "EndFrame: no acquired image index found for this instance. Did you call BeginFrame?");
            if (frameScope) {
                Logger::GetInstance().PopLogScope(frameScope);
                LogScopeManager::GetInstance().DestroyScope(frameScope, false);  // 出错，输出日志
            }
            return;
        }
        imageIndex = it->second;
        // 用完立即移除
        s_acquiredImageIndex.erase(it);
    }

    if (imageIndex >= swapChainImages.size()) {
        LOG_ERROR("RendererVulkan",
                  "EndFrame: stored imageIndex {0} out of range (count={1})",
                  imageIndex,
                  swapChainImages.size());
        if (frameScope) {
            Logger::GetInstance().PopLogScope(frameScope);
            LogScopeManager::GetInstance().DestroyScope(frameScope, false);  // 出错，输出日志
        }
        return;
    }

    // 调用 GUI 渲染回调
    if (m_guiRenderCallback) {
        m_guiRenderCallback(commandBuffers[imageIndex]);
    }

    // 结束 RenderPass 和 CommandBuffer
    vkCmdEndRenderPass(commandBuffers[imageIndex]);
    if (vkEndCommandBuffer(commandBuffers[imageIndex]) != VK_SUCCESS) {
        LOG_ERROR("RendererVulkan", "failed to record command buffer!");
        if (frameScope) {
            Logger::GetInstance().PopLogScope(frameScope);
            LogScopeManager::GetInstance().DestroyScope(frameScope, false);
        }
        return;
    }

    // 3) 提交命令缓冲区，使用与 imageIndex 对应的命令缓冲
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[]      = {imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount     = (imageAvailableSemaphore != VK_NULL_HANDLE) ? 1u : 0u;
    submitInfo.pWaitSemaphores        = (submitInfo.waitSemaphoreCount > 0) ? waitSemaphores : nullptr;
    submitInfo.pWaitDstStageMask      = (submitInfo.waitSemaphoreCount > 0) ? waitStages : nullptr;

    // 确保按 imageIndex 选择命令缓冲
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &commandBuffers[imageIndex];

    VkSemaphore signalSemaphores[]  = {renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = (renderFinishedSemaphore != VK_NULL_HANDLE) ? 1u : 0u;
    submitInfo.pSignalSemaphores    = (submitInfo.signalSemaphoreCount > 0) ? signalSemaphores : nullptr;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS) {
        LOG_ERROR("RendererVulkan", "vkQueueSubmit failed");
        if (frameScope) {
            Logger::GetInstance().PopLogScope(frameScope);
            LogScopeManager::GetInstance().DestroyScope(frameScope, false);  // 出错，输出日志
        }
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    // 4) 准备 Present，确保传入有效的 pImageIndices
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = submitInfo.signalSemaphoreCount;
    presentInfo.pWaitSemaphores    = submitInfo.pSignalSemaphores;

    VkSwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount  = 1;
    presentInfo.pSwapchains     = swapChains;
    presentInfo.pImageIndices   = &imageIndex;  // 关键：绝不能传入 nullptr 当 swapchainCount > 0 时

    // pResults 可选，置为 nullptr 保持兼容性
    presentInfo.pResults = nullptr;

    VkResult presentRes = vkQueuePresentKHR(graphicsQueue, &presentInfo);
    bool success        = true;
    if (presentRes == VK_ERROR_OUT_OF_DATE_KHR || presentRes == VK_SUBOPTIMAL_KHR) {
        LOG_WARNING("RendererVulkan",
                    "vkQueuePresentKHR returned {0} - swapchain may need recreation",
                    static_cast<int>(presentRes));
        // 这种情况不算严重错误，仍然认为帧渲染成功
    } else if (presentRes != VK_SUCCESS) {
        LOG_ERROR("RendererVulkan", "vkQueuePresentKHR failed: {0}", static_cast<int>(presentRes));
        success = false;
    }

    // 结束日志作用域
    if (frameScope) {
        Logger::GetInstance().PopLogScope(frameScope);
        LogScopeManager::GetInstance().DestroyScope(frameScope, success);  // 根据渲染结果决定是否输出日志
    }
}

void RendererVulkan::Resize(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) return;

    m_swapchainExtent.width = width;
    m_swapchainExtent.height = height;

    vkDeviceWaitIdle(device);

    // 清理旧的 swapchain 资源
    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    
    vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device, swapChain, nullptr);

    // 重建
    CreateSwapChain();
    CreateImageViews();
    CreateFramebuffers();
    CreateCommandBuffers();
    
    LOG_INFO("Vulkan", "Swapchain resized to {0}x{1}", width, height);
}

void RendererVulkan::SubmitRenderCommand(const RenderCommand& cmd) {
    // TODO: 实现具体的渲染命令提交
}

bool RendererVulkan::Supports(RendererFeature feature) const {
    return false;
}

void RendererVulkan::Present() {
    // 在EndFrame中已经实现了呈现逻辑
}

bool RendererVulkan::CreateInstance(const char* const* extensions, uint32_t extCount) {
    VkApplicationInfo appInfo{};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = "YAGE Engine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName        = "YAGE";
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo        = &appInfo;
    createInfo.ppEnabledExtensionNames = extensions;
    createInfo.enabledExtensionCount   = extCount;
    createInfo.enabledLayerCount       = 0;

    VkResult res = vkCreateInstance(&createInfo, nullptr, &instance);
    if (res != VK_SUCCESS) {
        LOG_ERROR("Vulkan", "vkCreateInstance failed: {0}", static_cast<int>(res));
        throw std::runtime_error("failed to create instance!");
    }
    return true;
}

void RendererVulkan::CreateSwapChain() {
    // 查询交换链支持信息
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_surface, &capabilities);

    swapChainImageFormat = VK_FORMAT_B8G8R8A8_SRGB;

    // 选择最佳 Present Mode
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        std::vector<VkPresentModeKHR> availablePresentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, m_surface, &presentModeCount, availablePresentModes.data());

        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
        }
    }

    // 确定图像数量 (通常 min + 1 以实现三缓冲)
    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    // 创建交换链
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType         = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface       = m_surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat   = swapChainImageFormat;
    createInfo.imageExtent   = m_swapchainExtent;

    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices    = FindQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value()};

    createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices   = nullptr;

    createInfo.preTransform   = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode    = presentMode;
    createInfo.clipped        = VK_TRUE;

    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    // 获取交换链图像
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
}

void RendererVulkan::CreateImageViews() {
    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapChainImages[i];

        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format   = swapChainImageFormat;

        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel   = 0;
        createInfo.subresourceRange.levelCount     = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount     = 1;

        if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image views!");
        }
    }
}

void RendererVulkan::CreateRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format         = swapChainImageFormat;
    colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;  // 清屏
    colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments    = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments    = &colorAttachment;
    renderPassInfo.subpassCount    = 1;
    renderPassInfo.pSubpasses      = &subpass;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

void RendererVulkan::CreateFramebuffers() {
    swapChainFramebuffers.resize(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        VkImageView attachments[] = {swapChainImageViews[i]};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass      = renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments    = attachments;
        framebufferInfo.width           = m_swapchainExtent.width;
        framebufferInfo.height          = m_swapchainExtent.height;
        framebufferInfo.layers          = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void RendererVulkan::CreateCommandPool() {
    QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

void RendererVulkan::CreateCommandBuffers() {
    commandBuffers.resize(swapChainFramebuffers.size());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool        = commandPool;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void RendererVulkan::PickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    } else {
        LOG_INFO("Vulkan", "找到 {0} 个支持 Vulkan 的物理设备", deviceCount);
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto& device : devices) {
        if (IsDeviceSuitable(device)) {
            physicalDevice = device;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    } else {
        LOG_INFO("Vulkan", "已选择合适的物理设备");
    }
}

void RendererVulkan::CreateLogicalDevice() {
    QueueFamilyIndices indices     = FindQueueFamilies(physicalDevice);
    const char* deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
    graphicsQueueFamily              = indices.graphicsFamily.value();
    queueCreateInfo.queueCount       = 1;

    float queuePriority              = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos       = &queueCreateInfo;
    createInfo.queueCreateInfoCount    = 1;
    createInfo.pEnabledFeatures        = &deviceFeatures;
    createInfo.ppEnabledExtensionNames = deviceExtensions;
    createInfo.enabledExtensionCount   = 1;
    createInfo.enabledLayerCount       = 0;

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
}

bool RendererVulkan::IsDeviceSuitable(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceFeatures.geometryShader;
}

QueueFamilyIndices RendererVulkan::FindQueueFamilies(VkPhysicalDevice device) {
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

        if (indices.IsComplete()) {
            break;
        }

        i++;
    }

    return indices;
}

}  // namespace Engine
