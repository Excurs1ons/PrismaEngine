#include "RenderBackendVulkan.h"
#include "../platform/ApplicationWindows.h"
#include "Camera2D.h"
#include "LogScope.h"
#include "Logger.h"
#include "SceneManager.h"
#include "RenderErrorHandling.h"
#include <DirectXMath.h>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

namespace Engine {

// Vulkan渲染命令上下文的改进实现
class VulkanRenderCommandContext : public RenderCommandContext {
public:
    VulkanRenderCommandContext(RenderBackendVulkan* backend) : m_backend(backend) {
        // 验证后端有效性
        if (!m_backend) {
            throw std::runtime_error("VulkanRenderCommandContext: Backend is null");
        }
    }

    void SetConstantBuffer(const char* name, FXMMATRIX matrix) override {
        if (!name) {
            LOG_WARNING("VulkanRenderCommand", "SetConstantBuffer called with null name");
            return;
        }

        // 存储矩阵数据
        XMFLOAT4X4 matrixData;
        XMStoreFloat4x4(&matrixData, matrix);

        // 在实际实现中，这里应该更新Vulkan uniform buffer
        m_constantBuffers[name] = std::vector<float>(
            reinterpret_cast<float*>(&matrixData),
            reinterpret_cast<float*>(&matrixData) + 16
        );

        LOG_DEBUG("VulkanRenderCommand", "Set constant buffer '{0}' with matrix data", name);
    }

    void SetConstantBuffer(const char* name, const float* data, size_t size) override {
        if (!name || !data) {
            LOG_WARNING("VulkanRenderCommand", "SetConstantBuffer called with null parameters");
            return;
        }

        if (size == 0) {
            LOG_WARNING("VulkanRenderCommand", "SetConstantBuffer called with zero size");
            return;
        }

        // 复制数据到常量缓冲区映射
        m_constantBuffers[name] = std::vector<float>(data, data + size / sizeof(float));

        LOG_DEBUG("VulkanRenderCommand", "Set constant buffer '{0}' with {1} bytes", name, size);
    }

    void SetVertexBuffer(const void* data, uint32_t sizeInBytes, uint32_t strideInBytes) override {
        if (!data || sizeInBytes == 0 || strideInBytes == 0) {
            LOG_WARNING("VulkanRenderCommand", "Invalid vertex buffer parameters");
            return;
        }

        m_vertexBufferData.resize(sizeInBytes);
        std::memcpy(m_vertexBufferData.data(), data, sizeInBytes);
        m_vertexStride = strideInBytes;

        LOG_DEBUG("VulkanRenderCommand", "Set vertex buffer: {0} bytes, stride {1}", sizeInBytes, strideInBytes);
    }

    void SetIndexBuffer(const void* data, uint32_t sizeInBytes, bool use16BitIndices = true) override {
        if (!data || sizeInBytes == 0) {
            LOG_WARNING("VulkanRenderCommand", "Invalid index buffer parameters");
            return;
        }

        m_indexBufferData.resize(sizeInBytes);
        std::memcpy(m_indexBufferData.data(), data, sizeInBytes);
        m_use16BitIndices = use16BitIndices;

        LOG_DEBUG("VulkanRenderCommand", "Set index buffer: {0} bytes, 16-bit: {1}", sizeInBytes, use16BitIndices);
    }

    void SetShaderResource(const char* name, void* resource) override {
        if (!name) {
            LOG_WARNING("VulkanRenderCommand", "SetShaderResource called with null name");
            return;
        }

        m_shaderResources[name] = resource;

        LOG_DEBUG("VulkanRenderCommand", "Set shader resource '{0}': 0x{1:x}",
                  name, reinterpret_cast<uintptr_t>(resource));
    }

    void SetSampler(const char* name, void* sampler) override {
        if (!name) {
            LOG_WARNING("VulkanRenderCommand", "SetSampler called with null name");
            return;
        }

        m_samplers[name] = sampler;

        LOG_DEBUG("VulkanRenderCommand", "Set sampler '{0}': 0x{1:x}",
                  name, reinterpret_cast<uintptr_t>(sampler));
    }

    void DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation = 0, uint32_t baseVertexLocation = 0) override {
        if (indexCount == 0) {
            LOG_WARNING("VulkanRenderCommand", "DrawIndexed called with zero index count");
            return;
        }

        if (m_indexBufferData.empty()) {
            LOG_ERROR("VulkanRenderCommand", "DrawIndexed called without index buffer");
            return;
        }

        // 在实际实现中，这里应该记录Vulkan绘制命令
        // 目前只记录调试信息
        LOG_DEBUG("VulkanRenderCommand", "DrawIndexed: {0} indices, start {1}, base vertex {2}",
                  indexCount, startIndexLocation, baseVertexLocation);
    }

    void Draw(uint32_t vertexCount, uint32_t startVertexLocation = 0) override {
        if (vertexCount == 0) {
            LOG_WARNING("VulkanRenderCommand", "Draw called with zero vertex count");
            return;
        }

        if (m_vertexBufferData.empty()) {
            LOG_ERROR("VulkanRenderCommand", "Draw called without vertex buffer");
            return;
        }

        // 在实际实现中，这里应该记录Vulkan绘制命令
        LOG_DEBUG("VulkanRenderCommand", "Draw: {0} vertices, start {1}", vertexCount, startVertexLocation);
    }

    void SetViewport(float x, float y, float width, float height) override {
        if (width <= 0 || height <= 0) {
            LOG_WARNING("VulkanRenderCommand", "Invalid viewport dimensions: {0}x{1}", width, height);
            return;
        }

        // 存储视口参数
        m_viewport.x = x;
        m_viewport.y = y;
        m_viewport.width = width;
        m_viewport.height = height;

        LOG_DEBUG("VulkanRenderCommand", "Set viewport: ({0},{1}) {2}x{3}", x, y, width, height);
    }

    void SetScissorRect(int left, int top, int right, int bottom) override {
        // 验证矩形有效性
        if (right <= left || bottom <= top) {
            LOG_WARNING("VulkanRenderCommand", "Invalid scissor rect: ({0},{1}) to ({2},{3})",
                        left, top, right, bottom);
            return;
        }

        m_scissorRect.left = left;
        m_scissorRect.top = top;
        m_scissorRect.right = right;
        m_scissorRect.bottom = bottom;

        LOG_DEBUG("VulkanRenderCommand", "Set scissor rect: ({0},{1}) to ({2},{3})",
                  left, top, right, bottom);
    }

private:
    RenderBackendVulkan* m_backend;

    // 存储渲染状态数据
    std::unordered_map<std::string, std::vector<float>> m_constantBuffers;
    std::unordered_map<std::string, void*> m_shaderResources;
    std::unordered_map<std::string, void*> m_samplers;

    std::vector<uint8_t> m_vertexBufferData;
    std::vector<uint8_t> m_indexBufferData;
    uint32_t m_vertexStride = 0;
    bool m_use16BitIndices = true;

    struct Viewport {
        float x, y, width, height;
    } m_viewport = {0.0f, 0.0f, 0.0f, 0.0f};

    struct ScissorRect {
        int left, top, right, bottom;
    } m_scissorRect = {0, 0, 0, 0};
};

// 用于在不修改头文件的情况下将每个实例的 acquired imageIndex 保持到 EndFrame
// 使用线程安全的容器避免额外的互斥锁开销
#include <atomic>
static thread_local std::unordered_map<void*, uint32_t> s_acquiredImageIndex;

RenderBackendVulkan::RenderBackendVulkan()
    : instance(VK_NULL_HANDLE), physicalDevice(VK_NULL_HANDLE), device(VK_NULL_HANDLE), graphicsQueue(VK_NULL_HANDLE),
      renderPass(VK_NULL_HANDLE), swapChain(VK_NULL_HANDLE), commandPool(VK_NULL_HANDLE),
      imageAvailableSemaphore(VK_NULL_HANDLE), renderFinishedSemaphore(VK_NULL_HANDLE), inFlightFence(VK_NULL_HANDLE) {}
bool RenderBackendVulkan::Initialize(Platform* platform, void* windowHandle, void* surface, uint32_t width, uint32_t height) {
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

void RenderBackendVulkan::Shutdown() {
    if (!isInitialized) {
        return; // 避免重复销毁
    }

    // 按照创建的逆序销毁资源
    if (inFlightFence != VK_NULL_HANDLE) {
        vkDestroyFence(device, inFlightFence, nullptr);
        inFlightFence = VK_NULL_HANDLE;
    }
    if (renderFinishedSemaphore != VK_NULL_HANDLE) {
        vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
        renderFinishedSemaphore = VK_NULL_HANDLE;
    }
    if (imageAvailableSemaphore != VK_NULL_HANDLE) {
        vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
        imageAvailableSemaphore = VK_NULL_HANDLE;
    }

    for (auto framebuffer : swapChainFramebuffers) {
        if (framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
    }
    swapChainFramebuffers.clear();

    if (commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device, commandPool, nullptr);
        commandPool = VK_NULL_HANDLE;
    }

    if (renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, renderPass, nullptr);
        renderPass = VK_NULL_HANDLE;
    }

    for (auto imageView : swapChainImageViews) {
        if (imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(device, imageView, nullptr);
        }
    }
    swapChainImageViews.clear();

    if (swapChain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device, swapChain, nullptr);
        swapChain = VK_NULL_HANDLE;
    }

    if (device != VK_NULL_HANDLE) {
        vkDestroyDevice(device, nullptr);
        device = VK_NULL_HANDLE;
    }

    if (instance != VK_NULL_HANDLE && m_surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance, m_surface, nullptr);
        m_surface = VK_NULL_HANDLE;
    }

    if (instance != VK_NULL_HANDLE) {
        vkDestroyInstance(instance, nullptr);
        instance = VK_NULL_HANDLE;
    }

    isInitialized = false;
    LOG_INFO("Vulkan", "Vulkan renderer shutdown completed");
}

void RenderBackendVulkan::BeginFrame(DirectX::XMFLOAT4 clearColor) {
    if (isFrameActive) {
        LOG_WARNING("RendererVulkan", "BeginFrame called while frame is already active");
        return;
    }

    // 创建渲染帧日志作用域
    LogScope* frameScope = LogScopeManager::GetInstance().CreateScope("VulkanFrame");
    Logger::GetInstance().PushLogScope(frameScope);

    // 等待上一帧完成
    vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &inFlightFence);

    isFrameActive = true; // 标记帧开始

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

    // 保存 imageIndex 以便 EndFrame 使用（线程本地存储，无需锁）
    s_acquiredImageIndex[this] = imageIndex;
    // 更新当前帧索引
    currentFrame = imageIndex;

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

    // 转换DirectXMath向量为Vulkan清除颜色
    VkClearValue vkClearColor = {{{clearColor.x, clearColor.y, clearColor.z, clearColor.w}}};

    LOG_DEBUG("RendererVulkan", "Using clear color: ({0}, {1}, {2}, {3})",
              clearColor.x, clearColor.y, clearColor.z, clearColor.w);

    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues    = &vkClearColor;

    vkCmdBeginRenderPass(commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // 在这里可以添加绘制命令，但现在只是清屏所以不需要
    // vkCmdDraw(...);

    // vkCmdEndRenderPass 和 vkEndCommandBuffer 移至 EndFrame
    // 正常结束BeginFrame，继续保持作用域活跃到EndFrame
}

void RenderBackendVulkan::EndFrame() {
    if (!isFrameActive) {
        LOG_WARNING("RendererVulkan", "EndFrame called without active BeginFrame");
        return;
    }

    // 获取当前日志作用域
    LogScope* frameScope = Logger::GetInstance().GetCurrentLogScope();

    // 1) 基本有效性检查 - 使用错误处理宏简化代码
    if (device == VK_NULL_HANDLE) {
        HANDLE_RENDER_ERROR("EndFrame: device is VK_NULL_HANDLE", frameScope);
    }
    if (swapChain == VK_NULL_HANDLE) {
        HANDLE_RENDER_ERROR("EndFrame: swapChain is VK_NULL_HANDLE", frameScope);
    }
    if (graphicsQueue == VK_NULL_HANDLE) {
        HANDLE_RENDER_ERROR("EndFrame: graphicsQueue is VK_NULL_HANDLE", frameScope);
    }
    if (imageAvailableSemaphore == VK_NULL_HANDLE) {
        LOG_WARNING("RendererVulkan", "EndFrame: imageAvailableSemaphore is VK_NULL_HANDLE");
    }
    if (renderFinishedSemaphore == VK_NULL_HANDLE) {
        LOG_WARNING("RendererVulkan", "EndFrame: renderFinishedSemaphore is VK_NULL_HANDLE");
    }
    if (commandBuffers.empty()) {
        HANDLE_RENDER_ERROR("EndFrame: no command buffers allocated", frameScope);
    }
    if (swapChainImages.empty()) {
        HANDLE_RENDER_ERROR("EndFrame: no swap chain images", frameScope);
    }

    // 从 BeginFrame 保存的位置获取 imageIndex —— 不要在 EndFrame 再次调用 vkAcquireNextImageKHR
    uint32_t imageIndex = UINT32_MAX;
    auto it = s_acquiredImageIndex.find(this);
    if (it == s_acquiredImageIndex.end()) {
        HANDLE_RENDER_ERROR(
            "EndFrame: no acquired image index found for this instance. Did you call BeginFrame?",
            frameScope);
    }
    imageIndex = it->second;
    // 用完立即移除
    s_acquiredImageIndex.erase(it);

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

    isFrameActive = false; // 标记帧结束
}

void RenderBackendVulkan::Resize(uint32_t width, uint32_t height) {
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

void RenderBackendVulkan::SubmitRenderCommand(const RenderCommand& cmd) {
    // TODO: 实现具体的渲染命令提交
}

bool RenderBackendVulkan::Supports(RendererFeature feature) const {
    return false;
}

void RenderBackendVulkan::Present() {
    // 在EndFrame中已经实现了呈现逻辑
}

RenderCommandContext* RenderBackendVulkan::CreateCommandContext()
{
    // 创建一个新的Vulkan渲染命令上下文实例
    return new VulkanRenderCommandContext(this);
}

bool RenderBackendVulkan::CreateInstance(const char* const* extensions, uint32_t extCount) {
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

void RenderBackendVulkan::CreateSwapChain() {
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

void RenderBackendVulkan::CreateImageViews() {
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

void RenderBackendVulkan::CreateRenderPass() {
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

void RenderBackendVulkan::CreateFramebuffers() {
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

void RenderBackendVulkan::CreateCommandPool() {
    QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

void RenderBackendVulkan::CreateCommandBuffers() {
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

void RenderBackendVulkan::PickPhysicalDevice() {
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

void RenderBackendVulkan::CreateLogicalDevice() {
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

bool RenderBackendVulkan::IsDeviceSuitable(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceFeatures.geometryShader;
}

QueueFamilyIndices RenderBackendVulkan::FindQueueFamilies(VkPhysicalDevice device) {
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

void* RenderBackendVulkan::GetDefaultRenderTarget()
{
    // Vulkan中渲染目标是framebuffer，返回当前framebuffer
    if (currentFrame < swapChainFramebuffers.size()) {
        return &swapChainFramebuffers[currentFrame];
    }
    return nullptr;
}

void* RenderBackendVulkan::GetDefaultDepthBuffer()
{
    // TODO: Vulkan 深度缓冲区尚未实现
    return nullptr;
}

void RenderBackendVulkan::GetRenderTargetSize(uint32_t& width, uint32_t& height)
{
    width = m_swapchainExtent.width;
    height = m_swapchainExtent.height;
}

}  // namespace Engine
