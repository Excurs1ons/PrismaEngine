#include "VulkanRTBackend.h"
#include "logger/Logger.h"
#include <cstring>
#include <algorithm>
#include <vector>

namespace Prisma::Graphic {

namespace {

template<typename T>
static T LoadRTFunc(VkDevice device, const char* name) noexcept {
    auto fn = reinterpret_cast<T>(vkGetDeviceProcAddr(device, name));
    if (!fn) {
        LOG_WARN("VulkanRT", "vkGetDeviceProcAddr failed for {}", name);
    }
    return fn;
}

#define LOAD_RT_FUNC(dev, name) \
    static auto name = LoadRTFunc<PFN_##name>(dev, #name)

}

bool VulkanRTBackend::Initialize(VkDevice device, VkPhysicalDevice physDev,
                                  VmaAllocator allocator, uint32_t graphicsQF) {
    m_device   = device;
    m_physDev  = physDev;
    m_allocator = allocator;
    m_graphicsQF = graphicsQF;

    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtProps{};
    rtProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

    VkPhysicalDeviceProperties2 props2{};
    props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    props2.pNext = &rtProps;
    vkGetPhysicalDeviceProperties2(physDev, &props2);

    m_shaderGroupHandleSize    = rtProps.shaderGroupHandleSize;
    m_shaderGroupBaseAlignment = rtProps.shaderGroupBaseAlignment;

    VkCommandPoolCreateInfo poolCI{};
    poolCI.sType           = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolCI.flags           = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
                             VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolCI.queueFamilyIndex = graphicsQF;

    if (vkCreateCommandPool(device, &poolCI, nullptr, &m_commandPool) != VK_SUCCESS) {
        LOG_WARN("VulkanRT", "Failed to create command pool");
        return false;
    }

    LOG_INFO("VulkanRT", "Initialized: handleSize={}, baseAlignment={}",
             m_shaderGroupHandleSize, m_shaderGroupBaseAlignment);
    return true;
}

void VulkanRTBackend::DestroyTLAS() {
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
}

void VulkanRTBackend::Shutdown() {
    vkDeviceWaitIdle(m_device);

    if (m_rtPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_rtPipeline, nullptr);
        m_rtPipeline = VK_NULL_HANDLE;
    }

    if (m_sbtBuffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(m_allocator, m_sbtBuffer, m_sbtAlloc);
        m_sbtBuffer = VK_NULL_HANDLE;
        m_sbtAlloc  = VK_NULL_HANDLE;
    }

    DestroyTLAS();

    if (m_instanceBuffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(m_allocator, m_instanceBuffer, m_instanceAlloc);
        m_instanceBuffer = VK_NULL_HANDLE;
        m_instanceAlloc  = VK_NULL_HANDLE;
    }

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

    if (m_scratchBuffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(m_allocator, m_scratchBuffer, m_scratchAlloc);
        m_scratchBuffer = VK_NULL_HANDLE;
        m_scratchAlloc  = VK_NULL_HANDLE;
        m_scratchSize   = 0;
    }

    if (m_commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(m_device, m_commandPool, nullptr);
        m_commandPool = VK_NULL_HANDLE;
    }

    if (m_rtPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device, m_rtPipelineLayout, nullptr);
        m_rtPipelineLayout = VK_NULL_HANDLE;
    }

    LOG_INFO("VulkanRT", "Shutdown complete");
}

void VulkanRTBackend::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
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
        LOG_WARN("VulkanRT", "CreateBuffer failed (size={})", (uint64_t)size);
        return;
    }

    if (address) {
        VkBufferDeviceAddressInfo bdaInfo{};
        bdaInfo.sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        bdaInfo.buffer = buffer;
        *address = vkGetBufferDeviceAddress(m_device, &bdaInfo);
    }
}

static bool ExecuteOneShot(VkDevice device, VkCommandPool pool, VkQueue queue,
                            VkCommandBuffer cmd) {
    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        LOG_WARN("VulkanRT", "ExecuteOneShot: vkEndCommandBuffer failed");
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
        LOG_WARN("VulkanRT", "ExecuteOneShot: vkCreateFence failed");
        return false;
    }

    VkResult res = vkQueueSubmit(queue, 1, &submitInfo, fence);
    if (res != VK_SUCCESS) {
        LOG_WARN("VulkanRT", "ExecuteOneShot: vkQueueSubmit failed ({})", (int)res);
        vkDestroyFence(device, fence, nullptr);
        return false;
    }

    res = vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
    if (res != VK_SUCCESS) {
        LOG_WARN("VulkanRT", "ExecuteOneShot: vkWaitForFences failed ({})", (int)res);
    }

    vkDestroyFence(device, fence, nullptr);
    return res == VK_SUCCESS;
}

VkAccelerationStructureKHR VulkanRTBackend::BuildBLAS(const BLASInput& input) {
    if (!input.vertices || input.vertexCount == 0 || !input.indices || input.indexCount == 0) {
        LOG_WARN("VulkanRT", "BuildBLAS: invalid input");
        return VK_NULL_HANDLE;
    }

    LOAD_RT_FUNC(m_device, vkGetAccelerationStructureBuildSizesKHR);
    LOAD_RT_FUNC(m_device, vkCreateAccelerationStructureKHR);
    LOAD_RT_FUNC(m_device, vkDestroyAccelerationStructureKHR);
    LOAD_RT_FUNC(m_device, vkGetAccelerationStructureDeviceAddressKHR);
    LOAD_RT_FUNC(m_device, vkCmdBuildAccelerationStructuresKHR);

    const VkDeviceSize vertexBufferSize = static_cast<VkDeviceSize>(input.vertexCount) * 12;
    const VkDeviceSize indexBufferSize  = static_cast<VkDeviceSize>(input.indexCount) * sizeof(uint32_t);

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
        LOG_WARN("VulkanRT", "BuildBLAS: failed to create geometry buffers");
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

    VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
    buildInfo.sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    buildInfo.type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    buildInfo.flags         = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    buildInfo.geometryCount = 1;
    buildInfo.pGeometries   = &geom;

    const uint32_t primCount = input.indexCount / 3;
    VkAccelerationStructureBuildSizesInfoKHR sizeInfo{};
    sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vkGetAccelerationStructureBuildSizesKHR(m_device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                            &buildInfo, &primCount, &sizeInfo);

    VkBuffer asBuffer = VK_NULL_HANDLE;
    VmaAllocation asAlloc = VK_NULL_HANDLE;
    CreateBuffer(sizeInfo.accelerationStructureSize,
                 VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 asBuffer, asAlloc);

    if (asBuffer == VK_NULL_HANDLE) {
        LOG_WARN("VulkanRT", "BuildBLAS: failed to create AS buffer");
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
        LOG_WARN("VulkanRT", "BuildBLAS: vkCreateAccelerationStructureKHR failed");
        vmaDestroyBuffer(m_allocator, asBuffer, asAlloc);
        vmaDestroyBuffer(m_allocator, vertexBuffer, vertexAlloc);
        vmaDestroyBuffer(m_allocator, indexBuffer, indexAlloc);
        return VK_NULL_HANDLE;
    }

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
        LOG_WARN("VulkanRT", "BuildBLAS: failed to allocate scratch buffer");
        vkDestroyAccelerationStructureKHR(m_device, as, nullptr);
        vmaDestroyBuffer(m_allocator, asBuffer, asAlloc);
        vmaDestroyBuffer(m_allocator, vertexBuffer, vertexAlloc);
        vmaDestroyBuffer(m_allocator, indexBuffer, indexAlloc);
        return VK_NULL_HANDLE;
    }

    VkCommandBufferAllocateInfo cmdAI{};
    cmdAI.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAI.commandPool        = m_commandPool;
    cmdAI.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAI.commandBufferCount = 1;

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(m_device, &cmdAI, &cmd) != VK_SUCCESS) {
        LOG_WARN("VulkanRT", "BuildBLAS: failed to allocate command buffer");
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

    if (vkEndCommandBuffer(cmd) != VK_SUCCESS) {
        LOG_WARN("VulkanRT", "BuildBLAS: vkEndCommandBuffer failed");
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
        LOG_WARN("VulkanRT", "BuildBLAS: execution failed");
        vkDestroyAccelerationStructureKHR(m_device, as, nullptr);
        vmaDestroyBuffer(m_allocator, asBuffer, asAlloc);
        vmaDestroyBuffer(m_allocator, vertexBuffer, vertexAlloc);
        vmaDestroyBuffer(m_allocator, indexBuffer, indexAlloc);
        vkFreeCommandBuffers(m_device, m_commandPool, 1, &cmd);
        return VK_NULL_HANDLE;
    }

    vkFreeCommandBuffers(m_device, m_commandPool, 1, &cmd);

    VkAccelerationStructureDeviceAddressInfoKHR addrInfo{};
    addrInfo.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    addrInfo.accelerationStructure = as;
    VkDeviceAddress asAddress = vkGetAccelerationStructureDeviceAddressKHR(m_device, &addrInfo);

    vmaDestroyBuffer(m_allocator, vertexBuffer, vertexAlloc);
    vmaDestroyBuffer(m_allocator, indexBuffer, indexAlloc);

    BLASEntry entry;
    entry.handle  = as;
    entry.buffer  = asBuffer;
    entry.alloc   = asAlloc;
    entry.address = asAddress;
    m_blasEntries.push_back(entry);

    LOG_DEBUG("VulkanRT", "Built BLAS: {} primitives, addr={}", primCount, (uint64_t)asAddress);
    return as;
}

uint64_t VulkanRTBackend::GetBLASDeviceAddress(VkAccelerationStructureKHR blas) const {
    for (const auto& entry : m_blasEntries) {
        if (entry.handle == blas) {
            return entry.address;
        }
    }
    LOG_WARN("VulkanRT", "GetBLASDeviceAddress: BLAS not found");
    return 0;
}

bool VulkanRTBackend::BuildTLAS(const std::vector<InstanceInput>& instances) {
    if (instances.empty()) {
        LOG_WARN("VulkanRT", "BuildTLAS: no instances");
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

    VkBuffer instanceBuffer = VK_NULL_HANDLE;
    VmaAllocation instanceAlloc = VK_NULL_HANDLE;
    CreateBuffer(instanceBufferSize,
                 VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                 instanceBuffer, instanceAlloc);

    if (instanceBuffer == VK_NULL_HANDLE) {
        LOG_WARN("VulkanRT", "BuildTLAS: failed to create instance buffer");
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
    vkGetAccelerationStructureBuildSizesKHR(m_device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                                            &buildInfo, &instanceCount, &sizeInfo);

    if (m_tlas != VK_NULL_HANDLE) {
        vkDestroyAccelerationStructureKHR(m_device, m_tlas, nullptr);
        m_tlas = VK_NULL_HANDLE;
    }
    if (m_tlasBuffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(m_allocator, m_tlasBuffer, m_tlasAlloc);
        m_tlasBuffer = VK_NULL_HANDLE;
        m_tlasAlloc  = VK_NULL_HANDLE;
    }

    CreateBuffer(sizeInfo.accelerationStructureSize,
                 VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 m_tlasBuffer, m_tlasAlloc);

    if (m_tlasBuffer == VK_NULL_HANDLE) {
        LOG_WARN("VulkanRT", "BuildTLAS: failed to create TLAS buffer");
        return false;
    }

    VkAccelerationStructureCreateInfoKHR asCI{};
    asCI.sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    asCI.buffer = m_tlasBuffer;
    asCI.size   = sizeInfo.accelerationStructureSize;
    asCI.type   = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

    if (vkCreateAccelerationStructureKHR(m_device, &asCI, nullptr, &m_tlas) != VK_SUCCESS) {
        LOG_WARN("VulkanRT", "BuildTLAS: vkCreateAccelerationStructureKHR failed");
        vmaDestroyBuffer(m_allocator, m_tlasBuffer, m_tlasAlloc);
        m_tlasBuffer = VK_NULL_HANDLE;
        m_tlasAlloc  = VK_NULL_HANDLE;
        m_tlas       = VK_NULL_HANDLE;
        return false;
    }

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
        LOG_WARN("VulkanRT", "BuildTLAS: no scratch buffer");
        DestroyTLAS();
        return false;
    }

    VkCommandBufferAllocateInfo cmdAI{};
    cmdAI.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAI.commandPool        = m_commandPool;
    cmdAI.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAI.commandBufferCount = 1;

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(m_device, &cmdAI, &cmd) != VK_SUCCESS) {
        LOG_WARN("VulkanRT", "BuildTLAS: command buffer alloc failed");
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
        LOG_WARN("VulkanRT", "BuildTLAS: end command buffer failed");
        vkFreeCommandBuffers(m_device, m_commandPool, 1, &cmd);
        DestroyTLAS();
        return false;
    }

    VkQueue queue = VK_NULL_HANDLE;
    vkGetDeviceQueue(m_device, m_graphicsQF, 0, &queue);

    if (!ExecuteOneShot(m_device, m_commandPool, queue, cmd)) {
        LOG_WARN("VulkanRT", "BuildTLAS: execution failed");
        vkFreeCommandBuffers(m_device, m_commandPool, 1, &cmd);
        DestroyTLAS();
        return false;
    }

    vkFreeCommandBuffers(m_device, m_commandPool, 1, &cmd);

    {
        VkAccelerationStructureDeviceAddressInfoKHR addrInfo{};
        addrInfo.sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        addrInfo.accelerationStructure = m_tlas;
        m_tlasAddress = vkGetAccelerationStructureDeviceAddressKHR(m_device, &addrInfo);
    }

    LOG_INFO("VulkanRT", "Built TLAS: {} instances, addr={}", instanceCount, (uint64_t)m_tlasAddress);
    return true;
}

bool VulkanRTBackend::UpdateTLASInstances(VkCommandBuffer cmd,
                                           const std::vector<InstanceInput>& instances) {
    if (m_tlas == VK_NULL_HANDLE) {
        LOG_WARN("VulkanRT", "UpdateTLASInstances: TLAS not built yet");
        return false;
    }
    if (instances.empty()) {
        LOG_WARN("VulkanRT", "UpdateTLASInstances: no instances");
        return false;
    }

    LOAD_RT_FUNC(m_device, vkCmdBuildAccelerationStructuresKHR);

    const uint32_t instanceCount = static_cast<uint32_t>(instances.size());

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
            LOG_WARN("VulkanRT", "UpdateTLASInstances: buffer alloc failed");
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

    LOAD_RT_FUNC(m_device, vkGetAccelerationStructureBuildSizesKHR);
    VkAccelerationStructureBuildSizesInfoKHR sizeInfo{};
    sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vkGetAccelerationStructureBuildSizesKHR(m_device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
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

bool VulkanRTBackend::CreateRTPipeline(const RTPipelineShaders& shaders,
                                        VkDescriptorSetLayout descSetLayout,
                                        VkPipelineLayout pipelineLayout) {
    if (!shaders.rgenCode || shaders.rgenSize == 0 ||
        !shaders.rchitCode || shaders.rchitSize == 0 ||
        !shaders.rmissCode || shaders.rmissSize == 0) {
        LOG_WARN("VulkanRT", "CreateRTPipeline: missing shader data");
        return false;
    }

    LOAD_RT_FUNC(m_device, vkCreateRayTracingPipelinesKHR);
    LOAD_RT_FUNC(m_device, vkGetRayTracingShaderGroupHandlesKHR);

    auto createShaderModule = [&](const void* code, size_t size) -> VkShaderModule {
        VkShaderModuleCreateInfo smCI{};
        smCI.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        smCI.codeSize = size;
        smCI.pCode    = static_cast<const uint32_t*>(code);

        VkShaderModule mod = VK_NULL_HANDLE;
        if (vkCreateShaderModule(m_device, &smCI, nullptr, &mod) != VK_SUCCESS) {
            LOG_WARN("VulkanRT", "CreateRTPipeline: failed to create shader module");
        }
        return mod;
    };

    VkShaderModule rgenModule  = createShaderModule(shaders.rgenCode, shaders.rgenSize);
    VkShaderModule rchitModule = createShaderModule(shaders.rchitCode, shaders.rchitSize);
    VkShaderModule rmissModule = createShaderModule(shaders.rmissCode, shaders.rmissSize);

    if (rgenModule == VK_NULL_HANDLE || rchitModule == VK_NULL_HANDLE || rmissModule == VK_NULL_HANDLE) {
        if (rgenModule)  vkDestroyShaderModule(m_device, rgenModule, nullptr);
        if (rchitModule) vkDestroyShaderModule(m_device, rchitModule, nullptr);
        if (rmissModule) vkDestroyShaderModule(m_device, rmissModule, nullptr);
        return false;
    }

    VkPipelineShaderStageCreateInfo rgenStage{};
    rgenStage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    rgenStage.stage  = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    rgenStage.module = rgenModule;
    rgenStage.pName  = "main";

    VkPipelineShaderStageCreateInfo rchitStage{};
    rchitStage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    rchitStage.stage  = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    rchitStage.module = rchitModule;
    rchitStage.pName  = "main";

    VkPipelineShaderStageCreateInfo rmissStage{};
    rmissStage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    rmissStage.stage  = VK_SHADER_STAGE_MISS_BIT_KHR;
    rmissStage.module = rmissModule;
    rmissStage.pName  = "main";

    const VkPipelineShaderStageCreateInfo stages[] = { rgenStage, rchitStage, rmissStage };

    VkRayTracingShaderGroupCreateInfoKHR rgenGroup{};
    rgenGroup.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    rgenGroup.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    rgenGroup.generalShader      = 0;
    rgenGroup.closestHitShader   = VK_SHADER_UNUSED_KHR;
    rgenGroup.anyHitShader       = VK_SHADER_UNUSED_KHR;
    rgenGroup.intersectionShader = VK_SHADER_UNUSED_KHR;

    VkRayTracingShaderGroupCreateInfoKHR hitGroup{};
    hitGroup.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    hitGroup.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    hitGroup.generalShader      = VK_SHADER_UNUSED_KHR;
    hitGroup.closestHitShader   = 1;
    hitGroup.anyHitShader       = VK_SHADER_UNUSED_KHR;
    hitGroup.intersectionShader = VK_SHADER_UNUSED_KHR;

    VkRayTracingShaderGroupCreateInfoKHR missGroup{};
    missGroup.sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    missGroup.type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    missGroup.generalShader      = 2;
    missGroup.closestHitShader   = VK_SHADER_UNUSED_KHR;
    missGroup.anyHitShader       = VK_SHADER_UNUSED_KHR;
    missGroup.intersectionShader = VK_SHADER_UNUSED_KHR;

    // groups 顺序决定 SBT 中 handle 的排列顺序：
    //   [0]=rgen → raygenRegion
    //   [1]=miss → missRegion  
    //   [2]=hit  → hitRegion
    // 必须与 vkCmdTraceRaysKHR 的 region 顺序一致
    const VkRayTracingShaderGroupCreateInfoKHR groups[] = { rgenGroup, missGroup, hitGroup };
    const uint32_t groupCount = 3;

    VkRayTracingPipelineCreateInfoKHR rtPipelineCI{};
    rtPipelineCI.sType               = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    rtPipelineCI.stageCount          = 3;
    rtPipelineCI.pStages             = stages;
    rtPipelineCI.groupCount          = groupCount;
    rtPipelineCI.pGroups             = groups;
    rtPipelineCI.maxPipelineRayRecursionDepth = 1;
    rtPipelineCI.layout              = pipelineLayout;
    rtPipelineCI.basePipelineHandle  = VK_NULL_HANDLE;
    rtPipelineCI.basePipelineIndex   = -1;

    VkPipeline pipeline = VK_NULL_HANDLE;
    VkResult res = vkCreateRayTracingPipelinesKHR(m_device, VK_NULL_HANDLE, VK_NULL_HANDLE,
                                                   1, &rtPipelineCI, nullptr, &pipeline);
    if (res != VK_SUCCESS) {
        LOG_WARN("VulkanRT", "CreateRTPipeline: vkCreateRayTracingPipelinesKHR failed ({})", (int)res);
        vkDestroyShaderModule(m_device, rgenModule, nullptr);
        vkDestroyShaderModule(m_device, rchitModule, nullptr);
        vkDestroyShaderModule(m_device, rmissModule, nullptr);
        return false;
    }

    vkDestroyShaderModule(m_device, rgenModule, nullptr);
    vkDestroyShaderModule(m_device, rchitModule, nullptr);
    vkDestroyShaderModule(m_device, rmissModule, nullptr);

    m_rtPipeline       = pipeline;
    m_rtPipelineLayout = pipelineLayout;

    const uint32_t handleSize    = m_shaderGroupHandleSize;
    const uint32_t baseAlignment = m_shaderGroupBaseAlignment;
    const uint32_t handleSizeAligned =
        (handleSize + baseAlignment - 1) & ~(baseAlignment - 1);

    const VkDeviceSize sbtSize = static_cast<VkDeviceSize>(groupCount) * handleSizeAligned;

    std::vector<uint8_t> handles(static_cast<size_t>(groupCount) * handleSize);
    res = vkGetRayTracingShaderGroupHandlesKHR(m_device, m_rtPipeline, 0, groupCount,
                                                handles.size(), handles.data());
    if (res != VK_SUCCESS) {
        LOG_WARN("VulkanRT", "CreateRTPipeline: vkGetRayTracingShaderGroupHandlesKHR failed ({})", (int)res);
        vkDestroyPipeline(m_device, m_rtPipeline, nullptr);
        m_rtPipeline = VK_NULL_HANDLE;
        return false;
    }

    VkBuffer sbtBuffer = VK_NULL_HANDLE;
    VmaAllocation sbtAlloc = VK_NULL_HANDLE;
    VkDeviceAddress sbtAddress = 0;
    CreateBuffer(sbtSize,
                 VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR |
                 VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                 sbtBuffer, sbtAlloc, &sbtAddress);

    if (sbtBuffer == VK_NULL_HANDLE) {
        LOG_WARN("VulkanRT", "CreateRTPipeline: failed to create SBT buffer");
        vkDestroyPipeline(m_device, m_rtPipeline, nullptr);
        m_rtPipeline = VK_NULL_HANDLE;
        return false;
    }

    m_sbtBuffer = sbtBuffer;
    m_sbtAlloc  = sbtAlloc;

    void* mapped = nullptr;
    if (vmaMapMemory(m_allocator, m_sbtAlloc, &mapped) == VK_SUCCESS) {
        auto* sbtData = static_cast<uint8_t*>(mapped);
        std::memset(sbtData, 0, static_cast<size_t>(sbtSize));

        for (uint32_t i = 0; i < groupCount; i++) {
            std::memcpy(sbtData + static_cast<size_t>(i) * handleSizeAligned,
                        handles.data() + static_cast<size_t>(i) * handleSize,
                        handleSize);
        }
        vmaUnmapMemory(m_allocator, m_sbtAlloc);
    }

    m_rgenRegion.deviceAddress = sbtAddress;
    m_rgenRegion.stride        = handleSizeAligned;
    m_rgenRegion.size          = handleSizeAligned;

    m_missRegion.deviceAddress = sbtAddress + static_cast<VkDeviceSize>(1) * handleSizeAligned;
    m_missRegion.stride        = handleSizeAligned;
    m_missRegion.size          = handleSizeAligned;

    m_hitRegion.deviceAddress = sbtAddress + static_cast<VkDeviceSize>(2) * handleSizeAligned;
    m_hitRegion.stride        = handleSizeAligned;
    m_hitRegion.size          = handleSizeAligned;

    m_callableRegion.deviceAddress = 0;
    m_callableRegion.stride        = 0;
    m_callableRegion.size          = 0;

    LOG_INFO("VulkanRT", "Created RT pipeline + SBT ({}/{} groups, handleSize={}, align={})",
             groupCount, groupCount, handleSize, baseAlignment);
    return true;
}

VkPipelineLayout VulkanRTBackend::CreatePipelineLayout(VkDescriptorSetLayout descSetLayout) {
    VkPipelineLayoutCreateInfo plCI{};
    plCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plCI.setLayoutCount = 1;
    plCI.pSetLayouts = &descSetLayout;
    plCI.pushConstantRangeCount = 0;
    plCI.pPushConstantRanges = nullptr;

    VkPipelineLayout pipeLayout = VK_NULL_HANDLE;
    if (vkCreatePipelineLayout(m_device, &plCI, nullptr, &pipeLayout) != VK_SUCCESS) {
        LOG_WARN("VulkanRT", "CreatePipelineLayout: failed");
        return VK_NULL_HANDLE;
    }
    m_rtPipelineLayout = pipeLayout;
    return pipeLayout;
}

void VulkanRTBackend::BindAndTraceRays(VkCommandBuffer cmd, uint32_t width, uint32_t height,
                                        VkDescriptorSet descSet) {
    if (m_rtPipeline == VK_NULL_HANDLE || descSet == VK_NULL_HANDLE) return;

    LOAD_RT_FUNC(m_device, vkCmdTraceRaysKHR);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_rtPipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                            m_rtPipelineLayout, 0, 1, &descSet, 0, nullptr);
    vkCmdTraceRaysKHR(cmd,
                      &m_rgenRegion, &m_missRegion, &m_hitRegion, &m_callableRegion,
                      width, height, 1);
}

}
