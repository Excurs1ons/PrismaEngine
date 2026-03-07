#include "VulkanAdapters.h"
#include "RenderDeviceVulkan.h"

namespace PrismaEngine::Graphic::Vulkan {

std::unique_ptr<RenderDeviceVulkan> CreateRenderDeviceVulkan(const DeviceDesc& deviceDesc) {
    auto device = std::make_unique<RenderDeviceVulkan>();
    if (device->Initialize(deviceDesc)) {
        return device;
    }
    return nullptr;
}

std::unique_ptr<IRenderDevice> CreateRenderDeviceVulkanInterface(const DeviceDesc& deviceDesc) {
    auto device = std::make_unique<RenderDeviceVulkan>();
    if (device->Initialize(deviceDesc)) {
        return device;
    }
    return nullptr;
}

} // namespace PrismaEngine::Graphic::Vulkan