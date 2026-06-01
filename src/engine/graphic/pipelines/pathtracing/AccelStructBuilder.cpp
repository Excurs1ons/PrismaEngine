#include "AccelStructBuilder.h"
#include "logger/Logger.h"
#include <cstring>
#include <algorithm>

namespace Prisma::Graphic {

namespace {

template<typename T>
static T LoadRTFunc(VkDevice device, const char* name) noexcept {
    auto fn = reinterpret_cast<T>(vkGetDeviceProcAddr(device, name));
    if (!fn) {
        LOG_WARN("AccelStructBuilder", "vkGetDeviceProcAddr failed for {}", name);
    }
    return fn;
}

#define LOAD_RT_FUNC(dev, name) \
    static auto name = LoadRTFunc<PFN_##name>(dev, #name)

static bool ExecuteOneShot(VkDevice device, VkCommandPool pool, VkQueue queue,
                            VkCommandBuffer cmd) {
    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        LOG_WARN("AccelStructBuilder", "ExecuteOneShot: vkEndCommandBuffer failed");
        return false;
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &cmd;

    VkFenceCreateInfo fenceCI{};
    fenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VkFence fence = VK_NULL_HANDLE;
    if (vkCreateFence(device, &fenceCI, nullptr, &fence) != VK_SUCCESS) {
        LOG_WARN("AccelStructBuilder", "ExecuteOneShot: vkCreateFence failed");
        return false;
    }

    VkResult res = vkQueueSubmit(queue, 1, &submitInfo, fence);
    if (res != VK_SUCCESS) {
        LOG_WARN("AccelStructBuilder", "ExecuteOneShot: vkQueueSubmit failed ({})", (int)res);
        vkDestroyFence(device, fence, nullptr);
        return false;
    }

    res = vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
    if (res != VK_SUCCESS) {
        LOG_WARN("AccelStructBuilder", "ExecuteOneShot: vkWaitForFences failed ({})", (int)res);
    }

    vkDestroyFence(device, fence, nullptr);
    return res == VK_SUCCESS;
}

}

bool AccelStructBuilder::Initialize(VkDevice device, VkPhysicalDevice physDev,
                                     VmaAllocator allocator, uint32_t graphicsQF) {
    m_device    = device;
    m_physDev   = physDev;
    m_allocator = allocator;
    m_graphicsQF = graphicsQF;

    VkCommandPoolCreateInfo poolCI{};
    poolCI.sType           = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolCI.flags           = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
                             VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolCI.queueFamilyIndex = graphicsQF;

    if (vkCreateCommandPool(device, &poolCI, nullptr, &m_commandPool) != VK_SUCCESS) {
        LOG_WARN("AccelStructBuilder", "Failed to create command pool");
        return false;
    }

    VkQueryPoolCreateInfo qpCI{};
    qpCI.sType     = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    qpCI.queryType = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
    qpCI.queryCount = 1;
    if (vkCreateQueryPool(device, &qpCI, nullptr, &m_queryPool) != VK_SUCCESS) {
        LOG_WARN("AccelStructBuilder", "Failed to create query pool for compaction");
        m_queryPool = VK_NULL_HANDLE;
    }

    LOG_INFO("AccelStructBuilder", "Initialized");
    return true;
}

void AccelStructBuilder::Shutdown() {
    if (m_device == VK_NULL_HANDLE) return;

    vkDeviceWaitIdle(m_device);

    // Destroy BLAS entries
    if (!m_blasEntries.empty()) {
        auto vkDestroyAccelerationStructureKHR =
            LoadRTFunc<PFN_vkDestroyAccelerationStructureKHR>(
                m_device, "vkDestroyAccelerationStructureKHR");
        for (auto& entry : m_blasEntries) {
            if (entry.handle != VK_NULL_HANDLE && vkDestroyAccelerationStructureKHR) {
                vkDestroyAccelerationStructureKHR(m_device, entry.handle, nullptr);
            }
            if (entry.buffer != VK_NULL_HANDLE) {
                vmaDestroyBuffer(m_allocator, entry.buffer, entry.alloc);
            }
        }
        m_blasEntries.clear();
    }

    DestroyTLAS();

    // Destroy instance buffer
    if (m_instanceBuffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(m_allocator, m_instanceBuffer, m_instanceAlloc);
        m_instanceBuffer = VK_NULL_HANDLE;
        m_instanceAlloc  = VK_NULL_HANDLE;
    }
    m_instanceCount   = 0;
    m_instanceAddress = 0;

    // Destroy scratch buffer
    if (m_scratchBuffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(m_allocator, m_scratchBuffer, m_scratchAlloc);
        m_scratchBuffer = VK_NULL_HANDLE;
        m_scratchAlloc  = VK_NULL_HANDLE;
        m_scratchSize   = 0;
        m_scratchAddress = 0;
    }

    // Destroy query pool
    if (m_queryPool != VK_NULL_HANDLE) {
        vkDestroyQueryPool(m_device, m_queryPool, nullptr);
        m_queryPool = VK_NULL_HANDLE;
    }

    // Destroy command pool
    if (m_commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(m_device, m_commandPool, nullptr);
        m_commandPool = VK_NULL_HANDLE;
    }

    m_device = VK_NULL_HANDLE;
    LOG_INFO("AccelStructBuilder", "Shutdown complete");
}

void AccelStructBuilder::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                       VkMemoryPropertyFlags memProps, VkBuffer& buffer,
                                       VmaAllocation& allocation, VkDeviceAddress* address) {
    buffer     = VK_NULL_HANDLE;
    allocation = VK_NULL_HANDLE;
    if (size == 0) return;

    VkBufferCreateInfo bufCI{};
    bufCI.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufCI.size        = size;
    bufCI.usage       = usage | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    bufCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocCI{};
    allocCI.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    if (memProps & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        allocCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                        VMA_ALLOCATION_CREATE_MAPPED_BIT;
    }

    if (vmaCreateBuffer(m_allocator, &bufCI, &allocCI, &buffer, &allocation, nullptr) != VK_SUCCESS) {
        LOG_WARN("AccelStructBuilder", "CreateBuffer failed (size={})", (uint64_t)size);
        return;
    }

    if (address) {
        VkBufferDeviceAddressInfo bdaInfo{};
        bdaInfo.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        bdaInfo.buffer = buffer;
        *address = vkGetBufferDeviceAddress(m_device, &bdaInfo);
    }
}

VkDeviceAddress AccelStructBuilder::GetBLASDeviceAddress(VkAccelerationStructureKHR blas) const {
    for (const auto& entry : m_blasEntries) {
        if (entry.handle == blas) {
            return entry.address;
        }
    }
    LOG_WARN("AccelStructBuilder", "GetBLASDeviceAddress: BLAS not found");
    return 0;
}

void AccelStructBuilder::DestroyTLAS() {
    if (m_tlas != VK_NULL_HANDLE) {
        auto vkDestroyAccelerationStructureKHR =
            LoadRTFunc<PFN_vkDestroyAccelerationStructureKHR>(
                m_device, "vkDestroyAccelerationStructureKHR");
        if (vkDestroyAccelerationStructureKHR) {
            vkDestroyAccelerationStructureKHR(m_device, m_tlas, nullptr);
        }
        m_tlas = VK_NULL_HANDLE;
    }
    if (m_tlasBuffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(m_allocator, m_tlasBuffer, m_tlasAlloc);
        m_tlasBuffer = VK_NULL_HANDLE;
        m_tlasAlloc  = VK_NULL_HANDLE;
    }
    m_tlasAddress = 0;
}

// ============================================================
// BuildBLAS — 构建带 compaction 的单 BLAS
// ============================================================
VkAccelerationStructureKHR AccelStructBuilder::BuildBLAS(const BLASInput& input) {
    if (!input.vertices || input.vertexCount == 0 || !input.indices || input.indexCount == 0) {
        LOG_WARN("AccelStructBuilder", "BuildBLAS: invalid input");
        return VK_NULL_HANDLE;
    }

    LOAD_RT_FUNC(m_device, vkGetAccelerationStructureBuildSizesKHR);
    LOAD_RT_FUNC(m_device, vkCreateAccelerationStructureKHR);
    LOAD_RT_FUNC(m_device, vkDestroyAccelerationStructureKHR);
    LOAD_RT_FUNC(m_device, vkGetAccelerationStructureDeviceAddressKHR);
    LOAD_RT_FUNC(m_device, vkCmdBuildAccelerationStructuresKHR);
    LOAD_RT_FUNC(m_device, vkCmdCopyAccelerationStructureKHR);
    LOAD_RT_FUNC(m_device, vkCmdWriteAccelerationStructuresPropertiesKHR);

    const VkDeviceSize vertexBufferSize = static_cast<VkDeviceSize>(input.vertexCount) * 12;
    const VkDeviceSize indexBufferSize  = static_cast<VkDeviceSize>(input.indexCount) * sizeof(uint32_t);

    // --- 1. Allocate and fill vertex/index buffers ---
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VmaAllocation vertexAlloc = VK_NULL_HANDLE;
    CreateBuffer(vertexBufferSize,
                 VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                 vertexBuffer, vertexAlloc);

    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VmaAllocation indexAlloc = VK_NULL_HANDLE;
    CreateBuffer(indexBufferSize,
                 VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                 indexBuffer, indexAlloc);

    if (vertexBuffer == VK_NULL_HANDLE || indexBuffer == VK_NULL_HANDLE) {
        LOG_WARN("AccelStructBuilder", "BuildBLAS: failed to create geometry buffers");
        if (vertexBuffer)  vmaDestroyBuffer(m_allocator, vertexBuffer, vertexAlloc);
        if (indexBuffer)   vmaDestroyBuffer(m_allocator, indexBuffer, indexAlloc);
        return VK_NULL_HANDLE;
    }

    void* mapped = nullptr;
    if (vmaMapMemory(m_allocator, vertexAlloc, &mapped) == VK_SUCCESS) {
        std::memcpy(mapped, input.vertices, static_cast<size_t>(vertexBufferSize));
        vmaUnmapMemory(m_allocator, vertexAlloc);
    }
    if (vmaMapMemory(m_allocator, indexAlloc, &mapped) == VK_SUCCESS) {
        std::memcpy(mapped, input.indices, static_cast<size_t>(indexBufferSize));
        vmaUnmapMemory(m_allocator, indexAlloc);
    }

    VkDeviceAddress vertexAddress = 0;
    {
        VkBufferDeviceAddressInfo bdaInfo{};
        bdaInfo.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        bdaInfo.buffer = vertexBuffer;
        vertexAddress = vkGetBufferDeviceAddress(m_device, &bdaInfo);
    }

    // --- 2. Geometry setup ---
    VkAccelerationStructureGeometryKHR geom{};
    geom.sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geom.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    geom.flags        = VK_GEOMETRY_OPAQUE_BIT_KHR;
    geom.geometry.triangles.sType =
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    geom.geometry.triangles.vertexFormat             = VK_FORMAT_R32G32B32_SFLOAT;
    geom.geometry.triangles.vertexData.deviceAddress  = vertexAddress;
    geom.geometry.triangles.vertexStride              = 12;
    geom.geometry.triangles.maxVertex                 = input.vertexCount - 1;
    geom.geometry.triangles.indexType                 = VK_INDEX_TYPE_UINT32;
    geom.geometry.triangles.indexData.deviceAddress   = 0;

    {
        VkBufferDeviceAddressInfo bdaInfo{};
        bdaInfo.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        bdaInfo.buffer = indexBuffer;
        VkDeviceAddress indexAddress = vkGetBufferDeviceAddress(m_device, &bdaInfo);
        geom.geometry.triangles.indexData.deviceAddress = indexAddress;
    }

    const uint32_t primCount = input.indexCount / 3;

    // --- 3. Build info with compaction flag ---
    VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
    buildInfo.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    buildInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    buildInfo.flags         = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR |
                              VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
    buildInfo.geometryCount = 1;
    buildInfo.pGeometries   = &geom;

    VkAccelerationStructureBuildSizesInfoKHR sizeInfo{};
    sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vkGetAccelerationStructureBuildSizesKHR(m_device,
                                            VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                            &buildInfo, &primCount, &sizeInfo);

    // --- 4. Allocate scratch buffer ---
    VkDeviceSize scratchNeeded = (std::max)(sizeInfo.buildScratchSize, sizeInfo.updateScratchSize);
    if (m_scratchSize < scratchNeeded) {
        if (m_scratchBuffer != VK_NULL_HANDLE) {
            vmaDestroyBuffer(m_allocator, m_scratchBuffer, m_scratchAlloc);
            m_scratchBuffer = VK_NULL_HANDLE;
            m_scratchAlloc  = VK_NULL_HANDLE;
            m_scratchSize   = 0;
        }
        CreateBuffer(scratchNeeded,
                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                     VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     m_scratchBuffer, m_scratchAlloc, &m_scratchAddress);
        if (m_scratchBuffer != VK_NULL_HANDLE) {
            m_scratchSize = scratchNeeded;
        }
    }

    if (m_scratchBuffer == VK_NULL_HANDLE) {
        LOG_WARN("AccelStructBuilder", "BuildBLAS: failed to allocate scratch buffer");
        vmaDestroyBuffer(m_allocator, vertexBuffer, vertexAlloc);
        vmaDestroyBuffer(m_allocator, indexBuffer, indexAlloc);
        return VK_NULL_HANDLE;
    }

    // --- 5. Allocate original (non-compacted) AS buffer + create AS ---
    VkBuffer asBuffer = VK_NULL_HANDLE;
    VmaAllocation asAlloc = VK_NULL_HANDLE;
    CreateBuffer(sizeInfo.accelerationStructureSize,
                 VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 asBuffer, asAlloc);

    if (asBuffer == VK_NULL_HANDLE) {
        LOG_WARN("AccelStructBuilder", "BuildBLAS: failed to create AS buffer");
        vmaDestroyBuffer(m_allocator, vertexBuffer, vertexAlloc);
        vmaDestroyBuffer(m_allocator, indexBuffer, indexAlloc);
        return VK_NULL_HANDLE;
    }

    VkAccelerationStructureCreateInfoKHR asCI{};
    asCI.sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    asCI.buffer = asBuffer;
    asCI.size   = sizeInfo.accelerationStructureSize;
    asCI.type   = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

    VkAccelerationStructureKHR as = VK_NULL_HANDLE;
    if (vkCreateAccelerationStructureKHR(m_device, &asCI, nullptr, &as) != VK_SUCCESS) {
        LOG_WARN("AccelStructBuilder", "BuildBLAS: vkCreateAccelerationStructureKHR failed");
        vmaDestroyBuffer(m_allocator, asBuffer, asAlloc);
        vmaDestroyBuffer(m_allocator, vertexBuffer, vertexAlloc);
        vmaDestroyBuffer(m_allocator, indexBuffer, indexAlloc);
        return VK_NULL_HANDLE;
    }

    // --- 6. Submit build ---
    VkCommandBufferAllocateInfo cmdAI{};
    cmdAI.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAI.commandPool        = m_commandPool;
    cmdAI.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAI.commandBufferCount = 1;

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(m_device, &cmdAI, &cmd) != VK_SUCCESS) {
        LOG_WARN("AccelStructBuilder", "BuildBLAS: failed to allocate command buffer");
        vkDestroyAccelerationStructureKHR(m_device, as, nullptr);
        vmaDestroyBuffer(m_allocator, asBuffer, asAlloc);
        vmaDestroyBuffer(m_allocator, vertexBuffer, vertexAlloc);
        vmaDestroyBuffer(m_allocator, indexBuffer, indexAlloc);
        return VK_NULL_HANDLE;
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    VkAccelerationStructureBuildRangeInfoKHR rangeInfo{};
    rangeInfo.primitiveCount  = primCount;
    rangeInfo.primitiveOffset = 0;
    rangeInfo.firstVertex     = 0;
    rangeInfo.transformOffset = 0;
    const VkAccelerationStructureBuildRangeInfoKHR* pRangeInfo = &rangeInfo;

    buildInfo.mode                     = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildInfo.dstAccelerationStructure  = as;
    buildInfo.scratchData.deviceAddress = m_scratchAddress;

    vkCmdBuildAccelerationStructuresKHR(cmd, 1, &buildInfo, &pRangeInfo);

    // Write compaction query after build
    if (m_queryPool != VK_NULL_HANDLE) {
        vkCmdResetQueryPool(cmd, m_queryPool, 0, 1);
        vkCmdWriteAccelerationStructuresPropertiesKHR(
            cmd, 1, &as,
            VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR,
            m_queryPool, 0);
    }

    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        LOG_WARN("AccelStructBuilder", "BuildBLAS: vkEndCommandBuffer failed");
        vkDestroyAccelerationStructureKHR(m_device, as, nullptr);
        vmaDestroyBuffer(m_allocator, asBuffer, asAlloc);
        vmaDestroyBuffer(m_allocator, vertexBuffer, vertexAlloc);
        vmaDestroyBuffer(m_allocator, indexBuffer, indexAlloc);
        vkFreeCommandBuffers(m_device, m_commandPool, 1, &cmd);
        return VK_NULL_HANDLE;
    }

    VkQueue queue = VK_NULL_HANDLE;
    vkGetDeviceQueue(m_device, m_graphicsQF, 0, &queue);

    if (!ExecuteOneShot(m_device, m_commandPool, queue, cmd)) {
        LOG_WARN("AccelStructBuilder", "BuildBLAS: build execution failed");
        vkDestroyAccelerationStructureKHR(m_device, as, nullptr);
        vmaDestroyBuffer(m_allocator, asBuffer, asAlloc);
        vmaDestroyBuffer(m_allocator, vertexBuffer, vertexAlloc);
        vmaDestroyBuffer(m_allocator, indexBuffer, indexAlloc);
        vkFreeCommandBuffers(m_device, m_commandPool, 1, &cmd);
        return VK_NULL_HANDLE;
    }

    vkFreeCommandBuffers(m_device, m_commandPool, 1, &cmd);

    // --- 7. Query compacted size ---
    VkDeviceSize compactSize = sizeInfo.accelerationStructureSize;
    if (m_queryPool != VK_NULL_HANDLE) {
        VkDeviceSize compactResult = 0;
        VkResult queryRes = vkGetQueryPoolResults(
            m_device, m_queryPool, 0, 1,
            sizeof(compactResult), &compactResult,
            sizeof(compactResult), VK_QUERY_RESULT_WAIT_BIT);
        if (queryRes == VK_SUCCESS && compactResult > 0 && compactResult < sizeInfo.accelerationStructureSize) {
            compactSize = compactResult;
        }
    }

    // --- 8. If compaction reduces size, create compact copy ---
    VkAccelerationStructureKHR finalAS = as;
    VkBuffer finalASBuffer = asBuffer;
    VmaAllocation finalASAlloc = asAlloc;

    if (compactSize < sizeInfo.accelerationStructureSize) {
        // Allocate compact buffer + AS
        VkBuffer compactBuffer = VK_NULL_HANDLE;
        VmaAllocation compactAlloc = VK_NULL_HANDLE;
        CreateBuffer(compactSize,
                     VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                     VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     compactBuffer, compactAlloc);

        if (compactBuffer == VK_NULL_HANDLE) {
            LOG_WARN("AccelStructBuilder", "BuildBLAS: failed to allocate compact buffer, using uncompacted");
        } else {
            VkAccelerationStructureCreateInfoKHR compactASCI{};
            compactASCI.sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
            compactASCI.buffer = compactBuffer;
            compactASCI.size   = compactSize;
            compactASCI.type   = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

            VkAccelerationStructureKHR compactAS = VK_NULL_HANDLE;
            if (vkCreateAccelerationStructureKHR(m_device, &compactASCI, nullptr, &compactAS) != VK_SUCCESS) {
                LOG_WARN("AccelStructBuilder", "BuildBLAS: compact AS creation failed, using uncompacted");
                vmaDestroyBuffer(m_allocator, compactBuffer, compactAlloc);
            } else {
                // Submit compact copy
                VkCommandBuffer cmd2 = VK_NULL_HANDLE;
                if (vkAllocateCommandBuffers(m_device, &cmdAI, &cmd2) == VK_SUCCESS) {
                    VkCommandBufferBeginInfo beginInfo2{};
                    beginInfo2.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                    beginInfo2.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
                    vkBeginCommandBuffer(cmd2, &beginInfo2);

                    VkCopyAccelerationStructureInfoKHR copyInfo{};
                    copyInfo.sType = VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR;
                    copyInfo.src   = as;
                    copyInfo.dst   = compactAS;
                    copyInfo.mode  = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;
                    vkCmdCopyAccelerationStructureKHR(cmd2, &copyInfo);

                    if (vkEndCommandBuffer(cmd2) == VK_SUCCESS) {
                        if (ExecuteOneShot(m_device, m_commandPool, queue, cmd2)) {
                            vkDestroyAccelerationStructureKHR(m_device, as, nullptr);
                            vmaDestroyBuffer(m_allocator, asBuffer, asAlloc);

                            finalAS       = compactAS;
                            finalASBuffer = compactBuffer;
                            finalASAlloc  = compactAlloc;
                        } else {
                            LOG_WARN("AccelStructBuilder", "BuildBLAS: compact copy failed, using uncompacted");
                            vkDestroyAccelerationStructureKHR(m_device, compactAS, nullptr);
                            vmaDestroyBuffer(m_allocator, compactBuffer, compactAlloc);
                        }
                    } else {
                        vkDestroyAccelerationStructureKHR(m_device, compactAS, nullptr);
                        vmaDestroyBuffer(m_allocator, compactBuffer, compactAlloc);
                    }
                    vkFreeCommandBuffers(m_device, m_commandPool, 1, &cmd2);
                } else {
                    vkDestroyAccelerationStructureKHR(m_device, compactAS, nullptr);
                    vmaDestroyBuffer(m_allocator, compactBuffer, compactAlloc);
                }
            }
        }
    }

    // --- 9. Clean up geometry staging buffers ---
    vmaDestroyBuffer(m_allocator, vertexBuffer, vertexAlloc);
    vmaDestroyBuffer(m_allocator, indexBuffer, indexAlloc);

    // --- 10. Get device address and store ---
    VkAccelerationStructureDeviceAddressInfoKHR addrInfo{};
    addrInfo.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    addrInfo.accelerationStructure = finalAS;
    VkDeviceAddress asAddress = vkGetAccelerationStructureDeviceAddressKHR(m_device, &addrInfo);

    BLASEntry entry;
    entry.handle       = finalAS;
    entry.buffer       = finalASBuffer;
    entry.alloc        = finalASAlloc;
    entry.address      = asAddress;
    entry.vertexCount  = input.vertexCount;
    entry.triangleCount = primCount;
    m_blasEntries.push_back(entry);

    LOG_DEBUG("AccelStructBuilder", "Built BLAS: {} triangles, compacted={}->{}, addr={}",
              primCount,
              static_cast<uint64_t>(sizeInfo.accelerationStructureSize),
              static_cast<uint64_t>(compactSize),
              static_cast<uint64_t>(asAddress));
    return finalAS;
}

// ============================================================
// BuildAllBLAS — 批量构建多 BLAS
// ============================================================
bool AccelStructBuilder::BuildAllBLAS(const std::vector<BLASInput>& inputs) {
    if (inputs.empty()) {
        LOG_WARN("AccelStructBuilder", "BuildAllBLAS: no inputs");
        return false;
    }

    // Pre-compute max scratch size
    LOAD_RT_FUNC(m_device, vkGetAccelerationStructureBuildSizesKHR);

    VkDeviceSize maxScratch = 0;
    for (const auto& input : inputs) {
        if (!input.vertices || input.vertexCount == 0 || !input.indices || input.indexCount == 0) {
            continue;
        }

        VkAccelerationStructureGeometryKHR geom{};
        geom.sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        geom.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        geom.flags        = VK_GEOMETRY_OPAQUE_BIT_KHR;
        geom.geometry.triangles.sType =
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        geom.geometry.triangles.vertexFormat    = VK_FORMAT_R32G32B32_SFLOAT;
        geom.geometry.triangles.vertexStride     = 12;
        geom.geometry.triangles.indexType        = VK_INDEX_TYPE_UINT32;

        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
        buildInfo.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        buildInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        buildInfo.flags         = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR |
                                  VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
        buildInfo.geometryCount = 1;
        buildInfo.pGeometries   = &geom;

        const uint32_t primCount = input.indexCount / 3;
        VkAccelerationStructureBuildSizesInfoKHR sizeInfo{};
        sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        vkGetAccelerationStructureBuildSizesKHR(m_device,
                                                VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                                &buildInfo, &primCount, &sizeInfo);

        maxScratch = (std::max)(maxScratch, sizeInfo.buildScratchSize);
    }

    // Allocate one scratch buffer for all builds
    if (m_scratchSize < maxScratch) {
        if (m_scratchBuffer != VK_NULL_HANDLE) {
            vmaDestroyBuffer(m_allocator, m_scratchBuffer, m_scratchAlloc);
            m_scratchBuffer = VK_NULL_HANDLE;
            m_scratchAlloc  = VK_NULL_HANDLE;
            m_scratchSize   = 0;
        }
        CreateBuffer(maxScratch,
                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                     VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     m_scratchBuffer, m_scratchAlloc, &m_scratchAddress);
        if (m_scratchBuffer != VK_NULL_HANDLE) {
            m_scratchSize = maxScratch;
        }
    }

    if (m_scratchBuffer == VK_NULL_HANDLE) {
        LOG_WARN("AccelStructBuilder", "BuildAllBLAS: no scratch buffer");
        return false;
    }

    // Build each BLAS
    bool allSucceeded = true;
    for (const auto& input : inputs) {
        VkAccelerationStructureKHR blas = BuildBLAS(input);
        if (blas == VK_NULL_HANDLE) {
            LOG_WARN("AccelStructBuilder", "BuildAllBLAS: one BLAS failed");
            allSucceeded = false;
        }
    }

    LOG_INFO("AccelStructBuilder", "BuildAllBLAS: {} entries (allSucceeded={})",
             m_blasEntries.size(), allSucceeded);
    return allSucceeded;
}

// ============================================================
// BuildTLAS — 构建顶层加速结构
// ============================================================
bool AccelStructBuilder::BuildTLAS(const std::vector<InstanceInput>& instances) {
    if (instances.empty()) {
        LOG_WARN("AccelStructBuilder", "BuildTLAS: no instances");
        return false;
    }

    LOAD_RT_FUNC(m_device, vkGetAccelerationStructureBuildSizesKHR);
    LOAD_RT_FUNC(m_device, vkCreateAccelerationStructureKHR);
    LOAD_RT_FUNC(m_device, vkDestroyAccelerationStructureKHR);
    LOAD_RT_FUNC(m_device, vkGetAccelerationStructureDeviceAddressKHR);
    LOAD_RT_FUNC(m_device, vkCmdBuildAccelerationStructuresKHR);

    const uint32_t instanceCount = static_cast<uint32_t>(instances.size());
    const VkDeviceSize instanceBufferSize =
        static_cast<VkDeviceSize>(instanceCount) * sizeof(VkAccelerationStructureInstanceKHR);

    // --- 1. Create instance buffer ---
    VkBuffer instanceBuffer = VK_NULL_HANDLE;
    VmaAllocation instanceAlloc = VK_NULL_HANDLE;
    CreateBuffer(instanceBufferSize,
                 VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                 instanceBuffer, instanceAlloc);

    if (instanceBuffer == VK_NULL_HANDLE) {
        LOG_WARN("AccelStructBuilder", "BuildTLAS: failed to create instance buffer");
        return false;
    }

    if (m_instanceBuffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(m_allocator, m_instanceBuffer, m_instanceAlloc);
    }
    m_instanceBuffer  = instanceBuffer;
    m_instanceAlloc   = instanceAlloc;
    m_instanceCount   = instanceCount;

    {
        VkBufferDeviceAddressInfo bdaInfo{};
        bdaInfo.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        bdaInfo.buffer = instanceBuffer;
        m_instanceAddress = vkGetBufferDeviceAddress(m_device, &bdaInfo);
    }

    // --- 2. Fill instance data ---
    void* mapped = nullptr;
    if (vmaMapMemory(m_allocator, instanceAlloc, &mapped) == VK_SUCCESS) {
        auto* dst = static_cast<VkAccelerationStructureInstanceKHR*>(mapped);
        for (uint32_t i = 0; i < instanceCount; i++) {
            std::memset(&dst[i], 0, sizeof(VkAccelerationStructureInstanceKHR));
            dst[i].transform                              = instances[i].transform;
            dst[i].instanceCustomIndex                    = instances[i].instanceCustomIndex & 0x00FFFFFF;
            dst[i].mask                                   = (i < instances.size()) ? instances[i].instanceMask : 0xFF;
            dst[i].instanceShaderBindingTableRecordOffset  = 0;
            dst[i].flags                                  = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
            dst[i].accelerationStructureReference          = instances[i].blasDeviceAddress;
        }
        vmaUnmapMemory(m_allocator, instanceAlloc);
    }

    // --- 3. Geometry info ---
    VkAccelerationStructureGeometryKHR geom{};
    geom.sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geom.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geom.flags        = VK_GEOMETRY_OPAQUE_BIT_KHR;
    geom.geometry.instances.sType =
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    geom.geometry.instances.arrayOfPointers = VK_FALSE;
    geom.geometry.instances.data.deviceAddress = m_instanceAddress;

    VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
    buildInfo.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    buildInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    buildInfo.flags         = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    buildInfo.geometryCount = 1;
    buildInfo.pGeometries   = &geom;

    VkAccelerationStructureBuildSizesInfoKHR sizeInfo{};
    sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vkGetAccelerationStructureBuildSizesKHR(m_device,
                                            VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                            &buildInfo, &instanceCount, &sizeInfo);

    // --- 4. Destroy old TLAS ---
    DestroyTLAS();

    // --- 5. Allocate TLAS buffer ---
    CreateBuffer(sizeInfo.accelerationStructureSize,
                 VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 m_tlasBuffer, m_tlasAlloc);

    if (m_tlasBuffer == VK_NULL_HANDLE) {
        LOG_WARN("AccelStructBuilder", "BuildTLAS: failed to create TLAS buffer");
        return false;
    }

    // --- 6. Create TLAS ---
    VkAccelerationStructureCreateInfoKHR asCI{};
    asCI.sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    asCI.buffer = m_tlasBuffer;
    asCI.size   = sizeInfo.accelerationStructureSize;
    asCI.type   = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

    if (vkCreateAccelerationStructureKHR(m_device, &asCI, nullptr, &m_tlas) != VK_SUCCESS) {
        LOG_WARN("AccelStructBuilder", "BuildTLAS: vkCreateAccelerationStructureKHR failed");
        vmaDestroyBuffer(m_allocator, m_tlasBuffer, m_tlasAlloc);
        m_tlasBuffer = VK_NULL_HANDLE;
        m_tlasAlloc  = VK_NULL_HANDLE;
        m_tlas       = VK_NULL_HANDLE;
        return false;
    }

    // --- 7. Ensure scratch buffer is large enough ---
    if (m_scratchSize < sizeInfo.buildScratchSize) {
        if (m_scratchBuffer != VK_NULL_HANDLE) {
            vmaDestroyBuffer(m_allocator, m_scratchBuffer, m_scratchAlloc);
            m_scratchBuffer = VK_NULL_HANDLE;
            m_scratchAlloc  = VK_NULL_HANDLE;
            m_scratchSize   = 0;
        }
        CreateBuffer(sizeInfo.buildScratchSize,
                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                     VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     m_scratchBuffer, m_scratchAlloc, &m_scratchAddress);
        if (m_scratchBuffer != VK_NULL_HANDLE) {
            m_scratchSize = sizeInfo.buildScratchSize;
        }
    }

    if (m_scratchBuffer == VK_NULL_HANDLE) {
        LOG_WARN("AccelStructBuilder", "BuildTLAS: no scratch buffer");
        DestroyTLAS();
        return false;
    }

    // --- 8. Build command buffer ---
    VkCommandBufferAllocateInfo cmdAI{};
    cmdAI.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAI.commandPool        = m_commandPool;
    cmdAI.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAI.commandBufferCount = 1;

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(m_device, &cmdAI, &cmd) != VK_SUCCESS) {
        LOG_WARN("AccelStructBuilder", "BuildTLAS: command buffer alloc failed");
        DestroyTLAS();
        return false;
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    VkAccelerationStructureBuildRangeInfoKHR rangeInfo{};
    rangeInfo.primitiveCount  = instanceCount;
    rangeInfo.primitiveOffset = 0;
    rangeInfo.firstVertex     = 0;
    rangeInfo.transformOffset = 0;
    const VkAccelerationStructureBuildRangeInfoKHR* pRangeInfo = &rangeInfo;

    buildInfo.mode                     = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildInfo.dstAccelerationStructure  = m_tlas;
    buildInfo.scratchData.deviceAddress = m_scratchAddress;

    vkCmdBuildAccelerationStructuresKHR(cmd, 1, &buildInfo, &pRangeInfo);

    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        LOG_WARN("AccelStructBuilder", "BuildTLAS: end command buffer failed");
        vkFreeCommandBuffers(m_device, m_commandPool, 1, &cmd);
        DestroyTLAS();
        return false;
    }

    VkQueue queue = VK_NULL_HANDLE;
    vkGetDeviceQueue(m_device, m_graphicsQF, 0, &queue);

    if (!ExecuteOneShot(m_device, m_commandPool, queue, cmd)) {
        LOG_WARN("AccelStructBuilder", "BuildTLAS: execution failed");
        vkFreeCommandBuffers(m_device, m_commandPool, 1, &cmd);
        DestroyTLAS();
        return false;
    }

    vkFreeCommandBuffers(m_device, m_commandPool, 1, &cmd);

    // --- 9. Get TLAS device address ---
    {
        VkAccelerationStructureDeviceAddressInfoKHR addrInfo{};
        addrInfo.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        addrInfo.accelerationStructure = m_tlas;
        m_tlasAddress = vkGetAccelerationStructureDeviceAddressKHR(m_device, &addrInfo);
    }

    LOG_INFO("AccelStructBuilder", "Built TLAS: {} instances, addr={}",
             instanceCount, static_cast<uint64_t>(m_tlasAddress));
    return true;
}

// ============================================================
// UpdateTLASInstances — 在已有 TLAS 上更新实例（外部 cmd）
// ============================================================
bool AccelStructBuilder::UpdateTLASInstances(VkCommandBuffer cmd,
                                              const std::vector<InstanceInput>& instances) {
    if (m_tlas == VK_NULL_HANDLE) {
        LOG_WARN("AccelStructBuilder", "UpdateTLASInstances: TLAS not built yet");
        return false;
    }
    if (instances.empty()) {
        LOG_WARN("AccelStructBuilder", "UpdateTLASInstances: no instances");
        return false;
    }

    LOAD_RT_FUNC(m_device, vkCmdBuildAccelerationStructuresKHR);

    const uint32_t instanceCount = static_cast<uint32_t>(instances.size());

    // Resize instance buffer if needed
    if (m_instanceCount != instanceCount || m_instanceBuffer == VK_NULL_HANDLE) {
        if (m_instanceBuffer != VK_NULL_HANDLE) {
            vmaDestroyBuffer(m_allocator, m_instanceBuffer, m_instanceAlloc);
        }

        const VkDeviceSize instanceBufferSize =
            static_cast<VkDeviceSize>(instanceCount) * sizeof(VkAccelerationStructureInstanceKHR);
        CreateBuffer(instanceBufferSize,
                     VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                     VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                     m_instanceBuffer, m_instanceAlloc);

        if (m_instanceBuffer == VK_NULL_HANDLE) {
            LOG_WARN("AccelStructBuilder", "UpdateTLASInstances: buffer alloc failed");
            return false;
        }

        VkBufferDeviceAddressInfo bdaInfo{};
        bdaInfo.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        bdaInfo.buffer = m_instanceBuffer;
        m_instanceAddress = vkGetBufferDeviceAddress(m_device, &bdaInfo);
        m_instanceCount = instanceCount;
    }

    void* mapped = nullptr;
    if (vmaMapMemory(m_allocator, m_instanceAlloc, &mapped) == VK_SUCCESS) {
        auto* dst = static_cast<VkAccelerationStructureInstanceKHR*>(mapped);
        for (uint32_t i = 0; i < instanceCount; i++) {
            std::memset(&dst[i], 0, sizeof(VkAccelerationStructureInstanceKHR));
            dst[i].transform                              = instances[i].transform;
            dst[i].instanceCustomIndex                    = instances[i].instanceCustomIndex & 0x00FFFFFF;
            dst[i].mask                                   = (i < instances.size()) ? instances[i].instanceMask : 0xFF;
            dst[i].instanceShaderBindingTableRecordOffset  = 0;
            dst[i].flags                                  = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
            dst[i].accelerationStructureReference          = instances[i].blasDeviceAddress;
        }
        vmaUnmapMemory(m_allocator, m_instanceAlloc);
    }

    // Set up geometry
    VkAccelerationStructureGeometryKHR geom{};
    geom.sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    geom.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geom.flags        = VK_GEOMETRY_OPAQUE_BIT_KHR;
    geom.geometry.instances.sType =
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    geom.geometry.instances.arrayOfPointers = VK_FALSE;
    geom.geometry.instances.data.deviceAddress = m_instanceAddress;

    VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
    buildInfo.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    buildInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    buildInfo.flags         = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    buildInfo.geometryCount = 1;
    buildInfo.pGeometries   = &geom;
    buildInfo.mode          = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildInfo.dstAccelerationStructure  = m_tlas;
    buildInfo.scratchData.deviceAddress = m_scratchAddress;

    // Ensure scratch is large enough
    LOAD_RT_FUNC(m_device, vkGetAccelerationStructureBuildSizesKHR);
    VkAccelerationStructureBuildSizesInfoKHR sizeInfo{};
    sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vkGetAccelerationStructureBuildSizesKHR(m_device,
                                            VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                            &buildInfo, &instanceCount, &sizeInfo);

    if (m_scratchSize < sizeInfo.buildScratchSize) {
        if (m_scratchBuffer != VK_NULL_HANDLE) {
            vmaDestroyBuffer(m_allocator, m_scratchBuffer, m_scratchAlloc);
            m_scratchBuffer = VK_NULL_HANDLE;
            m_scratchAlloc  = VK_NULL_HANDLE;
            m_scratchSize   = 0;
        }
        CreateBuffer(sizeInfo.buildScratchSize,
                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                     VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     m_scratchBuffer, m_scratchAlloc, &m_scratchAddress);
        if (m_scratchBuffer != VK_NULL_HANDLE) {
            m_scratchSize = sizeInfo.buildScratchSize;
        }
    }

    VkAccelerationStructureBuildRangeInfoKHR rangeInfo{};
    rangeInfo.primitiveCount  = instanceCount;
    rangeInfo.primitiveOffset = 0;
    rangeInfo.firstVertex     = 0;
    rangeInfo.transformOffset = 0;
    const VkAccelerationStructureBuildRangeInfoKHR* pRangeInfo = &rangeInfo;

    vkCmdBuildAccelerationStructuresKHR(cmd, 1, &buildInfo, &pRangeInfo);

    return true;
}

} // namespace Prisma::Graphic
