#include "VulkanAdapters.h"
#include "RenderDeviceVulkan.h"
#include "Logger.h"

namespace Prisma::Graphic::Vulkan {

std::unique_ptr<RenderDeviceVulkan> CreateRenderDeviceVulkan(const DeviceDesc& deviceDesc) {
    auto device = std::make_unique<RenderDeviceVulkan>();
    if (device->Initialize(deviceDesc)) {
        LOG_DEBUG("Adapter", "设备已初始化。");
        return device;
    }
    LOG_ERROR("Adapter", "设备初始化失败");
    return nullptr;
}

std::unique_ptr<IRenderDevice> CreateRenderDeviceVulkanInterface(const DeviceDesc& deviceDesc) {
    auto device = std::make_unique<RenderDeviceVulkan>();
    if (device->Initialize(deviceDesc)) {
        LOG_DEBUG("Adapter", "设备已初始化。");
        return device;
    }
    LOG_ERROR("Adapter", "设备初始化失败");
    return nullptr;
}

} // namespace Prisma::Graphic::Vulkan