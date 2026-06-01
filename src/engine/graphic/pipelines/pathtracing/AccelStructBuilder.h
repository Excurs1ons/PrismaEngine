#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <vector>
#include <cstdint>

namespace Prisma::Graphic {

/// 独立于 RT 管线的 BLAS/TLAS 构建工具
/// 可用于 ray query 路径，无需 RT pipeline 或 SBT
class AccelStructBuilder {
public:
    AccelStructBuilder() = default;
    ~AccelStructBuilder() { Shutdown(); }

    bool Initialize(VkDevice device, VkPhysicalDevice physDev, VmaAllocator allocator,
                    uint32_t graphicsQF);
    void Shutdown();

    // ======== BLAS 条目 ========
    struct BLASEntry {
        VkAccelerationStructureKHR handle = VK_NULL_HANDLE;
        VkBuffer buffer = VK_NULL_HANDLE;
        VmaAllocation alloc = VK_NULL_HANDLE;
        VkDeviceAddress address = 0;
        uint32_t vertexCount = 0;
        uint32_t triangleCount = 0;
    };

    // ======== BLAS ========
    struct BLASInput {
        const float* vertices;   // 顶点: interleaved pos[3] (stride = 12)
        uint32_t vertexCount;
        const uint32_t* indices; // 索引
        uint32_t indexCount;
    };

    /// 构建单个 BLAS（带 compaction）
    VkAccelerationStructureKHR BuildBLAS(const BLASInput& input);

    /// 批量构建所有 BLAS（共享 scratch buffer）
    bool BuildAllBLAS(const std::vector<BLASInput>& inputs);

    // ======== TLAS ========
    struct InstanceInput {
        VkTransformMatrixKHR transform;
        uint32_t instanceCustomIndex;
        uint64_t blasDeviceAddress;
        uint8_t instanceMask = 0xFF;
    };

    VkAccelerationStructureKHR GetTLAS() const { return m_tlas; }
    uint64_t GetTLASDeviceAddress() const { return m_tlasAddress; }
    VkAccelerationStructureKHR GetBLAS(size_t idx) const {
        return idx < m_blasEntries.size() ? m_blasEntries[idx].handle : VK_NULL_HANDLE;
    }

    const std::vector<BLASEntry>& GetBLASEntries() const { return m_blasEntries; }

    /// 构建 TLAS
    bool BuildTLAS(const std::vector<InstanceInput>& instances);

    /// 更新已存在的 TLAS 实例（外部 command buffer）
    bool UpdateTLASInstances(VkCommandBuffer cmd, const std::vector<InstanceInput>& instances);

    // ======== 内部清理 ========
    void DestroyTLAS();

    // ======== 地址查询 ========
    VkDeviceAddress GetBLASDeviceAddress(VkAccelerationStructureKHR blas) const;

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physDev = VK_NULL_HANDLE;
    VmaAllocator m_allocator = VK_NULL_HANDLE;
    uint32_t m_graphicsQF = 0;

    // BLAS 条目
    std::vector<BLASEntry> m_blasEntries;

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

    // Instance buffer
    VkBuffer m_instanceBuffer = VK_NULL_HANDLE;
    VmaAllocation m_instanceAlloc = VK_NULL_HANDLE;
    uint32_t m_instanceCount = 0;
    VkDeviceAddress m_instanceAddress = 0;

    // Query pool 用于 compaction
    VkQueryPool m_queryPool = VK_NULL_HANDLE;

    VkCommandPool m_commandPool = VK_NULL_HANDLE;

    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags memProps, VkBuffer& buffer,
                      VmaAllocation& allocation, VkDeviceAddress* address = nullptr);
};

} // namespace Prisma::Graphic
