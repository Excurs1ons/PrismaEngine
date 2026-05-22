#pragma once

#ifndef __ANDROID__

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <vector>
#include <cstdint>

namespace Prisma::Graphic {

/// Vulkan 硬件光线追踪后端
/// 管理 BLAS/TLAS 构建、RT 管线创建、SBT 生成和执行
class VulkanRTBackend {
public:
    VulkanRTBackend() = default;
    ~VulkanRTBackend() { Shutdown(); }

    bool Initialize(VkDevice device, VkPhysicalDevice physDev, VmaAllocator allocator,
                    uint32_t graphicsQF);
    void Shutdown();

    // ======== 单个 BLAS 条目 ========
    struct BLASEntry {
        VkAccelerationStructureKHR handle = VK_NULL_HANDLE;
        VkBuffer buffer = VK_NULL_HANDLE;
        VmaAllocation alloc = VK_NULL_HANDLE;
        VkDeviceAddress address = 0;
    };
    std::vector<BLASEntry> m_blasEntries;  // 外部可访问用于遍历计数

    // ======== BLAS (Bottom-Level Acceleration Structure) ========

    struct BLASInput {
        const float* vertices;   // 顶点数据: interleaved pos[3] (stride = 12 bytes)
        uint32_t vertexCount;
        const uint32_t* indices; // 索引数据
        uint32_t indexCount;
    };

    /// 从三角形网格构建 BLAS
    VkAccelerationStructureKHR BuildBLAS(const BLASInput& input);

    // ======== TLAS (Top-Level Acceleration Structure) ========

    struct InstanceInput {
        VkTransformMatrixKHR transform; // 对象世界变换
        uint32_t instanceCustomIndex;   // 对象索引
        uint64_t blasDeviceAddress;     // BLAS 设备地址
    };

    /// 构建 TLAS（包含所有实例）
    bool BuildTLAS(const std::vector<InstanceInput>& instances);

    /// 更新已存在的 TLAS 实例变换
    bool UpdateTLASInstances(VkCommandBuffer cmd, const std::vector<InstanceInput>& instances);

    VkAccelerationStructureKHR GetTLAS() const { return m_tlas; }
    uint64_t GetTLASDeviceAddress() const { return m_tlasAddress; }
    VkAccelerationStructureKHR GetBLAS(size_t idx) const { return idx < m_blasEntries.size() ? m_blasEntries[idx].handle : VK_NULL_HANDLE; }

    // ======== Ray Tracing Pipeline ========

    struct RTPipelineShaders {
        const void* rgenCode;
        size_t rgenSize;
        const void* rchitCode;
        size_t rchitSize;
        const void* rmissCode;
        size_t rmissSize;
    };

    bool CreateRTPipeline(const RTPipelineShaders& shaders,
                          VkDescriptorSetLayout descSetLayout,
                          VkPipelineLayout pipelineLayout);

    VkPipeline GetRTPipeline() const { return m_rtPipeline; }
    VkPipelineLayout GetRTPipelineLayout() const { return m_rtPipelineLayout; }

    // ======== Shader Binding Table ========

    const VkStridedDeviceAddressRegionKHR& GetRaygenRegion() const { return m_rgenRegion; }
    const VkStridedDeviceAddressRegionKHR& GetMissRegion()   const { return m_missRegion; }
    const VkStridedDeviceAddressRegionKHR& GetHitRegion()    const { return m_hitRegion; }
    const VkStridedDeviceAddressRegionKHR& GetCallableRegion() const { return m_callableRegion; }

    // ======== 管线布局创建 ========

    /// 从描述符集布局创建管线布局（存储到 m_rtPipelineLayout）
    VkPipelineLayout CreatePipelineLayout(VkDescriptorSetLayout descSetLayout);

    // ======== 执行 ========

    /// 绑定 RT 管线 + 描述符集，并执行 TraceRays
    void BindAndTraceRays(VkCommandBuffer cmd, uint32_t width, uint32_t height,
                          VkDescriptorSet descSet);

    // ======== 资源获取 ========

    VkDeviceAddress GetScratchAddress() const { return m_scratchAddress; }
    uint64_t GetBLASDeviceAddress(VkAccelerationStructureKHR blas) const;

    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags memProps, VkBuffer& buffer,
                      VmaAllocation& allocation, VkDeviceAddress* address = nullptr);

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physDev = VK_NULL_HANDLE;
    VmaAllocator m_allocator = VK_NULL_HANDLE;
    uint32_t m_graphicsQF = 0;

    // TLAS
    VkAccelerationStructureKHR m_tlas = VK_NULL_HANDLE;
    VkBuffer m_tlasBuffer = VK_NULL_HANDLE;
    VmaAllocation m_tlasAlloc = VK_NULL_HANDLE;
    VkDeviceAddress m_tlasAddress = 0;

    // Scratch buffer
    VkBuffer m_scratchBuffer = VK_NULL_HANDLE;
    VmaAllocation m_scratchAlloc = VK_NULL_HANDLE;
    VkDeviceAddress m_scratchAddress = 0;
    VkDeviceSize m_scratchSize = 0;

    // RT Pipeline
    VkPipeline m_rtPipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_rtPipelineLayout = VK_NULL_HANDLE;

    // Shader Binding Table
    VkBuffer m_sbtBuffer = VK_NULL_HANDLE;
    VmaAllocation m_sbtAlloc = VK_NULL_HANDLE;
    VkStridedDeviceAddressRegionKHR m_rgenRegion{};
    VkStridedDeviceAddressRegionKHR m_missRegion{};
    VkStridedDeviceAddressRegionKHR m_hitRegion{};
    VkStridedDeviceAddressRegionKHR m_callableRegion{};

    VkCommandPool m_commandPool = VK_NULL_HANDLE;

    VkBuffer m_instanceBuffer = VK_NULL_HANDLE;
    VmaAllocation m_instanceAlloc = VK_NULL_HANDLE;
    uint32_t m_instanceCount = 0;
    VkDeviceAddress m_instanceAddress = 0;

    uint32_t m_shaderGroupHandleSize = 0;
    uint32_t m_shaderGroupBaseAlignment = 1;
};

} // namespace Prisma::Graphic

#else // __ANDROID__ — stub (Vulkan 1.2 not available)

namespace Prisma::Graphic {
class VulkanRTBackend {
public:
    bool Initialize(...) { return false; }
    void Shutdown() {}
};
} // namespace Prisma::Graphic

#endif // __ANDROID__
