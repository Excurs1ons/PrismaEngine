#include "DX12Adapters.h"

namespace PrismaEngine::Graphic::DX12 {

std::unique_ptr<DX12RenderDevice> CreateDX12RenderDevice(const DeviceDesc& deviceDesc) {
    auto device = std::make_unique<DX12RenderDevice>();
    if (!device->Initialize(deviceDesc)) {
        return nullptr;
    }
    return device;
}

std::unique_ptr<IRenderDevice> CreateDX12RenderDeviceInterface(const DeviceDesc& deviceDesc) {
    auto device = CreateDX12RenderDevice(deviceDesc);
    if (!device) {
        return nullptr;
    }

    // 转换为接口指针
    return std::unique_ptr<IRenderDevice>(device.release());
}

} // namespace PrismaEngine::Graphic::DX12