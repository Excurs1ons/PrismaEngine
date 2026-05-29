#include "HeadlessRenderer.h"
#include "adapters/vulkan/RenderDeviceVulkan.h"
#include "logger/Logger.h"

namespace Prisma::Graphic {

HeadlessRenderer::HeadlessRenderer() {
    LOG_INFO("HeadlessRenderer", "HeadlessRenderer instance created");
}

HeadlessRenderer::~HeadlessRenderer() {
    Shutdown();
}

bool HeadlessRenderer::Init(const HeadlessRendererConfig& config) {
    m_config = config;

    LOG_INFO("HeadlessRenderer", "Initializing headless renderer: {}x{}, validation={}",
             config.width, config.height, config.enableValidation);

    // 1.) 创建设备描述，设置 headless=true
    DeviceDesc desc;
    desc.name              = "HeadlessRenderer";
    desc.width             = config.width;
    desc.height            = config.height;
    desc.enableValidation  = config.enableValidation;
    desc.presentMode       = config.presentMode;
    desc.headless          = true;

    // 2.) 创建 Vulkan 渲染设备（headless 模式）
    auto device = std::make_unique<Vulkan::RenderDeviceVulkan>();

    // 3.) 初始化设备 — RenderDeviceVulkan::Initialize 检测 headless=true
    //     后会跳过 surface/swapchain，转而创建离屏资源
    int result = device->Initialize(desc);
    if (result != 0) {
        LOG_ERROR("HeadlessRenderer", "Render device init failed: error={}", result);
        return false;
    }

    m_device = std::move(device);
    m_initialized = true;

    LOG_INFO("HeadlessRenderer", "Headless renderer initialized successfully");
    return true;
}

void HeadlessRenderer::Shutdown() {
    if (m_device) {
        m_device->Shutdown();
        m_device.reset();
    }
    m_initialized = false;
    LOG_INFO("HeadlessRenderer", "Headless renderer shut down");
}

void HeadlessRenderer::RenderFrame() {
    if (!m_initialized || !m_device)
        return;

    m_device->BeginFrame();

    // 执行默认清除 render pass（确保离屏目标有有效内容）
    if (!m_device->IsSwapChainRenderPassActive()) {
        m_device->BeginSwapChainRenderPass({0.1f, 0.1f, 0.1f, 1.0f});
        m_device->EndSwapChainRenderPass();
    }

    m_device->EndFrame();
    m_device->Present();
}

HeadlessReadbackResult HeadlessRenderer::ReadbackPixels(uint32_t x, uint32_t y,
                                                        uint32_t w, uint32_t h) {
    HeadlessReadbackResult result;
    result.width    = w;
    result.height   = h;
    result.channels = 4;

    if (!m_initialized || !m_device) {
        LOG_WARNING("HeadlessRenderer", "ReadbackPixels: renderer not initialized");
        return result;
    }

    auto* vkDevice = static_cast<Vulkan::RenderDeviceVulkan*>(m_device.get());
    if (!vkDevice->IsHeadless()) {
        LOG_WARNING("HeadlessRenderer", "ReadbackPixels: only supported in headless mode");
        return result;
    }

    // 离屏颜色图像
    VkImage colorImage = vkDevice->GetHeadlessColorImage();
    if (colorImage == VK_NULL_HANDLE) {
        LOG_WARNING("HeadlessRenderer", "ReadbackPixels: offscreen color image is null");
        return result;
    }

    // 分配 RGBA8 像素缓冲区
    size_t pixelSize = static_cast<size_t>(w) * h * 4;
    result.pixels.resize(pixelSize);

    bool success = vkDevice->ReadbackImage(
        colorImage,
        w, h,
        VK_FORMAT_B8G8R8A8_UNORM,
        result.pixels.data(),
        pixelSize
    );

    if (!success) {
        LOG_WARNING("HeadlessRenderer", "ReadbackPixels: pixel readback failed");
        result.pixels.clear();
    }

    return result;
}

} // namespace Prisma::Graphic
