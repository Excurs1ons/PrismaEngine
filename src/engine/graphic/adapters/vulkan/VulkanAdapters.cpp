#include "VulkanAdapters.h"
#include "RenderDeviceVulkan.h"
#include "Logger.h"

namespace Prisma::Graphic::Vulkan {

std::unique_ptr<RenderDeviceVulkan> CreateRenderDeviceVulkan(const DeviceDesc& deviceDesc) {
    auto device = std::make_unique<RenderDeviceVulkan>();
    if (device->Initialize(deviceDesc)) {
        LOG_INFO("Adapter", "Device Initialized.");
        return device;
    }
    LOG_ERROR("Adapter", "Device Initialize Failed");
    return nullptr;
}

std::unique_ptr<IRenderDevice> CreateRenderDeviceVulkanInterface(const DeviceDesc& deviceDesc) {
    auto device = std::make_unique<RenderDeviceVulkan>();
    if (device->Initialize(deviceDesc)) {
        LOG_INFO("Adapter", "Device Initialized.");
        return device;
    }
    LOG_ERROR("Adapter", "Device Initialize Failed");
    return nullptr;
}

} // namespace Prisma::Graphic::Vulkan