#pragma once

#include <gmock/gmock.h>
#include <cstdint>
#include <string>
#include <memory>

namespace Prisma::Graphic {

// 模拟渲染设备能力
struct MockDeviceCapabilities {
    bool supportsMultiThreaded = true;
    bool supportsBindlessTextures = false;
    bool supportsComputeShader = true;
    bool supportsRayTracing = false;
    bool supportsMeshShader = false;
    std::string gpuName = "Mock GPU";
    uint64_t totalMemory = 8589934592; // 8GB
};

// 模拟资源句柄
struct MockResourceHandle {
    uint64_t id = 0;
    std::string name;
};

/**
 * @brief 手动构建的 Mock IRenderDevice
 *
 * 不依赖 IRenderDevice.h（避免引入 Vulkan 头文件）。
 * 仅包含测试需要的方法，使用 GMock MOCK_METHOD 宏。
 *
 * 方法列表依据测试需求裁剪：
 * - CreateMesh / CreateMaterial / CreateShader : 模拟图形资源创建
 * - GetCapabilities : 返回设备能力信息
 * - CreateVertexBuffer / CreateIndexBuffer : 模拟缓冲区创建
 */
class MockRenderDevice {
public:
    virtual ~MockRenderDevice() = default;

    // ── 资源创建方法 ──
    MOCK_METHOD(MockResourceHandle, CreateMesh,
                (const std::string& name, const void* vertexData,
                 uint64_t vertexSize, const void* indexData,
                 uint64_t indexSize),
                ());

    MOCK_METHOD(MockResourceHandle, CreateMaterial,
                (const std::string& name), ());

    MOCK_METHOD(MockResourceHandle, CreateShader,
                (const std::string& name, const std::string& source,
                 const std::string& entryPoint),
                ());

    // ── 设备能力查询 ──
    MOCK_METHOD(MockDeviceCapabilities, GetCapabilities, (), (const));

    // ── 缓冲区创建 ──
    MOCK_METHOD(MockResourceHandle, CreateVertexBuffer,
                (uint64_t size, const void* data, bool dynamic), ());

    MOCK_METHOD(MockResourceHandle, CreateIndexBuffer,
                (uint64_t size, const void* data, bool dynamic), ());
};

} // namespace Prisma::Graphic
