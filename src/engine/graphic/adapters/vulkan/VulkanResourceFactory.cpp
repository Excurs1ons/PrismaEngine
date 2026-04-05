#include "VulkanResourceFactory.h"
#include "RenderDeviceVulkan.h"
#include "VulkanResources.h"
#include "VulkanShader.h"
#include "VulkanPipelineState.h"
#include "VulkanSampler.h"
#include "VulkanSwapChain.h"
#include "VulkanFence.h"
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include <vk_mem_alloc.h>
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
#include "logger/Logger.h"
#include <fstream>

namespace Prisma::Graphic::Vulkan {

// -----------------------------------------------------------------------
// [修复] TextureFormatToVkFormat
//
// 目的：
//   提供引擎 TextureFormat → VkFormat 的安全映射。
//
// 问题根源：
//   原代码对 imageInfo.format 直接做 static_cast<VkFormat>(desc.format)。
//   TextureFormat 是引擎自定义枚举，其整数值与 VkFormat 规范定义的
//   整数值并不一致（例如 TextureFormat::RGBA8_UNorm == 33，
//   而 VK_FORMAT_R8G8B8A8_UNORM == 37）。直接强转会把错误的格式枚举
//   传给 Vulkan 驱动，触发驱动内部的非法格式断言，导致 nvoglv64.dll 崩溃。
//
// 修复过程：
//   新增此函数，用 switch-case 为每个引擎格式显式返回对应的 VkFormat
//   常量，unknown/未支持格式返回 VK_FORMAT_UNDEFINED 以便上层拦截。
// -----------------------------------------------------------------------
static VkFormat TextureFormatToVkFormat(TextureFormat format) {
    switch (format) {
        case TextureFormat::R8_UNorm:            return VK_FORMAT_R8_UNORM;
        case TextureFormat::R8_SNorm:            return VK_FORMAT_R8_SNORM;
        case TextureFormat::R8_UInt:             return VK_FORMAT_R8_UINT;
        case TextureFormat::R8_SInt:             return VK_FORMAT_R8_SINT;
        case TextureFormat::RG8_UNorm:           return VK_FORMAT_R8G8_UNORM;
        case TextureFormat::RG8_SNorm:           return VK_FORMAT_R8G8_SNORM;
        case TextureFormat::R16_UNorm:           return VK_FORMAT_R16_UNORM;
        case TextureFormat::R16_SNorm:           return VK_FORMAT_R16_SNORM;
        case TextureFormat::R16_Float:           return VK_FORMAT_R16_SFLOAT;
        case TextureFormat::R16_UInt:            return VK_FORMAT_R16_UINT;
        case TextureFormat::R16_SInt:            return VK_FORMAT_R16_SINT;
        case TextureFormat::RG16_UNorm:          return VK_FORMAT_R16G16_UNORM;
        case TextureFormat::RG16_SNorm:          return VK_FORMAT_R16G16_SNORM;
        case TextureFormat::RG16_Float:          return VK_FORMAT_R16G16_SFLOAT;
        case TextureFormat::RG16_UInt:           return VK_FORMAT_R16G16_UINT;
        case TextureFormat::RG16_SInt:           return VK_FORMAT_R16G16_SINT;
        case TextureFormat::RGBA16_UNorm:        return VK_FORMAT_R16G16B16A16_UNORM;
        case TextureFormat::RGBA16_SNorm:        return VK_FORMAT_R16G16B16A16_SNORM;
        case TextureFormat::RGBA16_Float:        return VK_FORMAT_R16G16B16A16_SFLOAT;
        case TextureFormat::RGBA16_UInt:         return VK_FORMAT_R16G16B16A16_UINT;
        case TextureFormat::RGBA16_SInt:         return VK_FORMAT_R16G16B16A16_SINT;
        case TextureFormat::R32_Float:           return VK_FORMAT_R32_SFLOAT;
        case TextureFormat::R32_UInt:            return VK_FORMAT_R32_UINT;
        case TextureFormat::R32_SInt:            return VK_FORMAT_R32_SINT;
        case TextureFormat::RG32_Float:          return VK_FORMAT_R32G32_SFLOAT;
        case TextureFormat::RG32_UInt:           return VK_FORMAT_R32G32_UINT;
        case TextureFormat::RG32_SInt:           return VK_FORMAT_R32G32_SINT;
        case TextureFormat::RGB32_Float:         return VK_FORMAT_R32G32B32_SFLOAT;
        case TextureFormat::RGB32_UInt:          return VK_FORMAT_R32G32B32_UINT;
        case TextureFormat::RGB32_SInt:          return VK_FORMAT_R32G32B32_SINT;
        case TextureFormat::RGBA32_Float:        return VK_FORMAT_R32G32B32A32_SFLOAT;
        case TextureFormat::RGBA32_UInt:         return VK_FORMAT_R32G32B32A32_UINT;
        case TextureFormat::RGBA32_SInt:         return VK_FORMAT_R32G32B32A32_SINT;
        case TextureFormat::RGB8_UNorm:          return VK_FORMAT_R8G8B8_UNORM;
        case TextureFormat::RGBA8_UNorm:         return VK_FORMAT_R8G8B8A8_UNORM;
        case TextureFormat::RGBA8_UNorm_sRGB:    return VK_FORMAT_R8G8B8A8_SRGB;
        case TextureFormat::RGBA8_SNorm:         return VK_FORMAT_R8G8B8A8_SNORM;
        case TextureFormat::RGBA8_UInt:          return VK_FORMAT_R8G8B8A8_UINT;
        case TextureFormat::RGBA8_SInt:          return VK_FORMAT_R8G8B8A8_SINT;
        case TextureFormat::BGRA8_UNorm:         return VK_FORMAT_B8G8R8A8_UNORM;
        case TextureFormat::BGRA8_UNorm_sRGB:    return VK_FORMAT_B8G8R8A8_SRGB;
        case TextureFormat::D16_UNorm:           return VK_FORMAT_D16_UNORM;
        case TextureFormat::D24_UNorm_S8_UInt:   return VK_FORMAT_D24_UNORM_S8_UINT;
        case TextureFormat::D32_Float:           return VK_FORMAT_D32_SFLOAT;
        case TextureFormat::D32_Float_S8_UInt:   return VK_FORMAT_D32_SFLOAT_S8_UINT;
        case TextureFormat::BC1_UNorm:           return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
        case TextureFormat::BC1_SRGB:            return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
        case TextureFormat::BC2_UNorm:           return VK_FORMAT_BC2_UNORM_BLOCK;
        case TextureFormat::BC2_SRGB:            return VK_FORMAT_BC2_SRGB_BLOCK;
        case TextureFormat::BC3_UNorm:           return VK_FORMAT_BC3_UNORM_BLOCK;
        case TextureFormat::BC3_SRGB:            return VK_FORMAT_BC3_SRGB_BLOCK;
        case TextureFormat::BC4_UNorm:           return VK_FORMAT_BC4_UNORM_BLOCK;
        case TextureFormat::BC4_SNorm:           return VK_FORMAT_BC4_SNORM_BLOCK;
        case TextureFormat::BC5_UNorm:           return VK_FORMAT_BC5_UNORM_BLOCK;
        case TextureFormat::BC5_SNorm:           return VK_FORMAT_BC5_SNORM_BLOCK;
        case TextureFormat::BC7_UNorm:           return VK_FORMAT_BC7_UNORM_BLOCK;
        case TextureFormat::BC7_SRGB:            return VK_FORMAT_BC7_SRGB_BLOCK;
        default:
            return VK_FORMAT_UNDEFINED;
    }
}

VulkanResourceFactory::VulkanResourceFactory(RenderDeviceVulkan* device)
    : m_device(device), m_vkDevice(device->GetVkDevice()), m_vmaAllocator(device->GetAllocator()) {
    LOG_INFO("Vulkan", "创建 Vulkan 资源工厂实例");
}

VulkanResourceFactory::~VulkanResourceFactory() {
    Shutdown();
}

bool VulkanResourceFactory::Initialize(IRenderDevice* device) {
    m_device = dynamic_cast<RenderDeviceVulkan*>(device);
    if (m_device) {
        m_vkDevice = m_device->GetVkDevice();
        m_vmaAllocator = m_device->GetAllocator();
    }
    return true;
}

void VulkanResourceFactory::Shutdown() {
    if (m_vkDevice != VK_NULL_HANDLE) {
        // [修复] 检查设备是否仍有效
        // 原因：如果 RenderDeviceVulkan 已先行销毁了 VkDevice，
        //       此处的 vkDeviceWaitIdle 会触发 Invalid Device 错误。
        //       虽然现在我们已通过在 RenderDeviceVulkan::Shutdown 中显式重置 Factory 来解决，
        //       但此处增加检查可进一步增强鲁棒性。
        
        // 我们无法简单检查 handle 释放，但可以依赖 m_device 的状态
        if (m_device && m_device->GetVkDevice() != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(m_vkDevice);
        }
    }
    m_device = nullptr;
    m_vkDevice = VK_NULL_HANDLE;
    m_vmaAllocator = VK_NULL_HANDLE;
}

void VulkanResourceFactory::Reset() {
}

std::unique_ptr<ITexture> VulkanResourceFactory::CreateTextureImpl(const TextureDesc& desc) {
    std::string errorMsg;
    if (!ValidateTextureDesc(desc, errorMsg)) {
        LOG_ERROR("Vulkan", "Invalid texture description: {0}", errorMsg);
        return nullptr;
    }

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = static_cast<uint32_t>(desc.width);
    imageInfo.extent.height = static_cast<uint32_t>(desc.height);
    imageInfo.extent.depth = desc.depth > 0 ? desc.depth : 1;
    imageInfo.mipLevels = desc.mipLevels > 0 ? desc.mipLevels : 1;
    imageInfo.arrayLayers = desc.arraySize;

    // [修复] 格式转换：使用 TextureFormatToVkFormat() 而不是 static_cast。
    //   目的：保证传给 vkCreateImage 的 VkFormat 值与 Vulkan 规范完全一致。
    //   过程：调用映射函数；若返回 VK_FORMAT_UNDEFINED，说明遇到未支持格式，
    //         提前返回 nullptr 而不是让驱动以非法格式建图后崩溃。
    imageInfo.format = TextureFormatToVkFormat(desc.format);
    if (imageInfo.format == VK_FORMAT_UNDEFINED) {
        LOG_ERROR("Vulkan", "Unsupported or unknown TextureFormat: {0}", static_cast<int>(desc.format));
        return nullptr;
    }

    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // [修复] VkImageUsageFlags 组装
    //
    // 目的：根据 TextureDesc 中的语义标志，生成符合 Vulkan 规范的 usage 位掩码。
    //   Vulkan 要求 CreateImage 时的 usage 必须覆盖后续所有实际用途，
    //   若遗漏某个 bit，驱动会在第一次实际使用时产生 validation error 或崩溃。
    //
    // 过程：
    //   1. 先按引擎语义标志设置主用途位。
    //   2. 为所有纹理统一追加 TRANSFER_DST_BIT：
    //        原代码只对 RT/DS 类型追加 TRANSFER_DST，导致 allowShaderResource=true
    //        的普通纹理无法通过 staging buffer 上传初始数据，Vulkan validation
    //        会报 VUID-vkCmdCopyBufferToImage-dstImage-00177 并在某些驱动上崩溃。
    //   3. RT/DS 额外追加 TRANSFER_SRC_BIT，以支持 readback/blit/resolve 操作。
    //   4. 若最终 usage 仅有 TRANSFER_DST（调用方未设置任何用途标志），
    //        则兜底追加 SAMPLED_BIT 并输出警告——纯传输目标在引擎中没有实际意义，
    //        这通常说明调用方的 TextureDesc 填写有误。
    imageInfo.usage = 0;

    if (desc.allowRenderTarget)    imageInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (desc.allowDepthStencil)    imageInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    if (desc.allowShaderResource)  imageInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if (desc.allowUnorderedAccess) imageInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;

    // 所有纹理均需 TRANSFER_DST 以支持从 staging buffer 上传数据
    imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    // RT/DS 纹理额外需要 TRANSFER_SRC 以支持截图（readback）和 MSAA resolve
    if (desc.allowRenderTarget || desc.allowDepthStencil)
        imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    // 兜底：确保纹理至少有一个实际用途，否则创建该纹理毫无意义
    if (imageInfo.usage == VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
        LOG_WARNING("Vulkan", "Texture has no usage flags other than TRANSFER_DST; defaulting to SAMPLED");
        imageInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }

    // [修复] sampleCount 保护
    //   目的：VkSampleCountFlagBits 必须是 2 的幂次（1/2/4/8/16/32/64），
    //         若 desc.sampleCount 为 0（默认值未初始化），直接强转会产生非法值。
    //   过程：0 值统一视为 1（不开启多重采样），与 TextureDesc 的语义一致。
    const uint32_t sc = desc.sampleCount > 0 ? desc.sampleCount : 1;
    imageInfo.samples = static_cast<VkSampleCountFlagBits>(sc);
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;

    // [修复] 错误诊断日志
    //   目的：原代码在 vmaCreateImage 失败时直接 return nullptr，
    //         没有任何诊断信息，极难定位崩溃来源。
    //   过程：失败时记录纹理名称、尺寸和引擎格式值，便于排查格式/尺寸非法等问题。
    if (vmaCreateImage(m_vmaAllocator, &imageInfo, &allocInfo, &image, &allocation, nullptr) != VK_SUCCESS) {
        LOG_ERROR("Vulkan", "vmaCreateImage failed for texture '{0}' ({1}x{2} fmt={3})",
            desc.name, desc.width, desc.height, static_cast<int>(desc.format));
        return nullptr;
    }

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = desc.arraySize > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = imageInfo.format;
    viewInfo.subresourceRange.aspectMask = desc.allowDepthStencil ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = imageInfo.mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = desc.arraySize;

    VkImageView imageView = VK_NULL_HANDLE;
    if (vkCreateImageView(m_vkDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        vmaDestroyImage(m_vmaAllocator, image, allocation);
        return nullptr;
    }

    auto texture = std::make_unique<VulkanTexture>(m_vkDevice, m_vmaAllocator, image, allocation, imageView, imageInfo.format, desc);

    // -----------------------------------------------------------------------
    // [改动] 初始布局转换
    //
    // 目的：
    //   解决新创建纹理处于 UNDEFINED 布局。若 ImGui 立即尝试采样绘制，会触发
    //   VUID-vkCmdDraw-None-09600 验证报错。
    //
    // 过程：
    //   1. 检查纹理是否允许作为着色器资源（ShaderResource）。
    //   2. 创建一个临时、即时提交的命令缓冲区（Command Buffer）。
    //   3. 插入一个 ImageMemoryBarrier，将布局从 UNDEFINED 转换为 GENERAL。
    //   4. 提交命令并调用 vkQueueWaitIdle 确保转换完成。
    //   虽然此操作涉及同步等待，但纹理创建（如视口 Resize）是低频操作，
    //   以此换取验证层稳定性和程序正确性是值得的。
    // -----------------------------------------------------------------------
    if (desc.allowShaderResource) {
        VkCommandPool tempPool;
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = m_device->GetGraphicsQueueFamily();
        poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        
        if (vkCreateCommandPool(m_vkDevice, &poolInfo, nullptr, &tempPool) == VK_SUCCESS) {
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = tempPool;
            allocInfo.commandBufferCount = 1;

            VkCommandBuffer commandBuffer;
            if (vkAllocateCommandBuffers(m_vkDevice, &allocInfo, &commandBuffer) == VK_SUCCESS) {
                VkCommandBufferBeginInfo beginInfo{};
                beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

                vkBeginCommandBuffer(commandBuffer, &beginInfo);

                VkImageMemoryBarrier barrier{};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL; // 使用 GENERAL 以确保最大的兼容性
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.image = image;
                barrier.subresourceRange.aspectMask = desc.allowDepthStencil ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = imageInfo.mipLevels;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = desc.arraySize;
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                vkCmdPipelineBarrier(
                    commandBuffer,
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier
                );

                vkEndCommandBuffer(commandBuffer);

                VkSubmitInfo submitInfo{};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &commandBuffer;

                vkQueueSubmit(m_device->GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
                vkQueueWaitIdle(m_device->GetGraphicsQueue());

                vkFreeCommandBuffers(m_vkDevice, tempPool, 1, &commandBuffer);
            }
            vkDestroyCommandPool(m_vkDevice, tempPool, nullptr);
        }
    }

    ++m_creationStats.texturesCreated;
    const uint64_t estimatedBytes = static_cast<uint64_t>(desc.width) * desc.height *
                                    std::max<uint32_t>(1, desc.depth) *
                                    std::max<uint32_t>(1, desc.arraySize) * 4ull;
    m_creationStats.totalMemoryAllocated += estimatedBytes;
    m_creationStats.peakMemoryUsage = std::max(m_creationStats.peakMemoryUsage, m_creationStats.totalMemoryAllocated);
    return texture;
}

std::unique_ptr<ITexture> VulkanResourceFactory::CreateTextureFromFile(const std::string& filename, const TextureDesc* desc) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) {
        LOG_ERROR("Vulkan", "Failed to open texture file: {0}", filename);
        return nullptr;
    }

    const auto fileSize = static_cast<uint64_t>(file.tellg());
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> bytes(static_cast<size_t>(fileSize));
    if (fileSize > 0) {
        file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(fileSize));
    }

    TextureDesc resolvedDesc = desc ? *desc : TextureDesc{};
    if (resolvedDesc.width == 0) {
        resolvedDesc.width = 1;
    }
    if (resolvedDesc.height == 0) {
        resolvedDesc.height = 1;
    }
    if (resolvedDesc.depth == 0) {
        resolvedDesc.depth = 1;
    }
    if (resolvedDesc.arraySize == 0) {
        resolvedDesc.arraySize = 1;
    }
    if (resolvedDesc.mipLevels == 0) {
        resolvedDesc.mipLevels = 1;
    }
    resolvedDesc.allowShaderResource = true;

    return CreateTextureFromMemory(bytes.data(), bytes.size(), resolvedDesc);
}

std::unique_ptr<ITexture> VulkanResourceFactory::CreateTextureFromMemory(const void* data, uint64_t dataSize, const TextureDesc& desc) {
    if (!data || dataSize == 0) {
        LOG_ERROR("Vulkan", "CreateTextureFromMemory 要求源数据不能为空");
        return nullptr;
    }

    return CreateTextureImpl(desc);
}

std::unique_ptr<IBuffer> VulkanResourceFactory::CreateBufferImpl(const BufferDesc& desc) {
    std::string errorMsg;
    if (!ValidateBufferDesc(desc, errorMsg)) {
        LOG_ERROR("Vulkan", "无效的缓冲区描述: {0}", errorMsg);
        return nullptr;
    }

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = desc.size;
    bufferInfo.usage = 0;

    // 根据 BufferType 设置 Vulkan buffer usage
    switch (desc.type) {
        case BufferType::Vertex:
            bufferInfo.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            break;
        case BufferType::Index:
            bufferInfo.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            break;
        case BufferType::Constant:
            bufferInfo.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            break;
        case BufferType::Structured:
            bufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            break;
        case BufferType::IndirectArgument:
            bufferInfo.usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
            break;
        default:
            break;
    }

    // 根据 BufferUsage 添加额外的 usage flags
    if (static_cast<uint32_t>(desc.usage) & static_cast<uint32_t>(BufferUsage::ShaderResource))
        bufferInfo.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;

    if (vmaCreateBuffer(m_vmaAllocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr) != VK_SUCCESS) return nullptr;

    // [修复] 处理初始数据上传
    if (desc.initialData && desc.size > 0) {
        void* mappedData = nullptr;
        if (vmaMapMemory(m_vmaAllocator, allocation, &mappedData) == VK_SUCCESS) {
            memcpy(mappedData, desc.initialData, desc.size);
            vmaUnmapMemory(m_vmaAllocator, allocation);
        } else {
            LOG_ERROR("Vulkan", "无法映射内存以进行初始数据上传");
        }
    }

    auto createdBuffer = std::make_unique<VulkanBuffer>(m_vmaAllocator, buffer, allocation, desc);
    ++m_creationStats.buffersCreated;
    m_creationStats.totalMemoryAllocated += desc.size;
    m_creationStats.peakMemoryUsage = std::max(m_creationStats.peakMemoryUsage, m_creationStats.totalMemoryAllocated);
    return createdBuffer;
}

std::unique_ptr<IBuffer> VulkanResourceFactory::CreateDynamicBuffer(uint64_t size, BufferType type, BufferUsage usage) {
    BufferDesc desc{};
    desc.size = size;
    desc.type = type;
    desc.usage = usage;
    return CreateBufferImpl(desc);
}

std::unique_ptr<IShader> VulkanResourceFactory::CreateShaderImpl(const ShaderDesc& desc, const std::vector<uint8_t>& bytecode, const ShaderReflection& reflection) {
    // 将 uint8_t 字节码转换为 uint32_t SPIR-V
    std::vector<uint32_t> spirv;
    if (bytecode.size() % 4 == 0) {
        spirv.resize(bytecode.size() / 4);
        memcpy(spirv.data(), bytecode.data(), bytecode.size());
    }
    ++m_creationStats.shadersCreated;
    return std::make_unique<VulkanShader>(m_device, desc, spirv, reflection);
}

std::unique_ptr<IPipelineState> VulkanResourceFactory::CreatePipelineStateImpl() {
    ++m_creationStats.pipelinesCreated;
    return std::make_unique<VulkanPipelineState>();
}

std::unique_ptr<ISampler> VulkanResourceFactory::CreateSamplerImpl(const SamplerDesc& desc) {
    ++m_creationStats.samplersCreated;
    return std::make_unique<VulkanSampler>(m_vkDevice, desc);
}

std::unique_ptr<ISwapChain> VulkanResourceFactory::CreateSwapChainImpl(void* windowHandle, uint32_t width, uint32_t height, TextureFormat format, uint32_t bufferCount, bool vsync) {
    if (!m_device || !windowHandle || width == 0 || height == 0) {
        return nullptr;
    }

    if (format != TextureFormat::RGBA8_UNorm) {
        LOG_WARNING("Vulkan", "Swapchain format override is not supported yet, falling back to RGBA8_UNorm");
    }
    if (bufferCount != 0 && bufferCount != 3) {
        LOG_WARNING("Vulkan", "Swapchain buffer count override is not supported yet, requested: {0}", bufferCount);
    }

    auto swapChain = std::make_unique<VulkanSwapChain>(m_device);
    if (swapChain->Initialize(windowHandle, width, height, vsync) != 0) {
        return nullptr;
    }
    return swapChain;
}
std::unique_ptr<IFence> VulkanResourceFactory::CreateFenceImpl() { return std::make_unique<VulkanFence>(m_vkDevice); }

std::vector<std::unique_ptr<ITexture>> VulkanResourceFactory::CreateTexturesBatch(const TextureDesc* descs, uint32_t count) {
    std::vector<std::unique_ptr<ITexture>> results;
    for (uint32_t i = 0; i < count; ++i) {
        auto texture = CreateTextureImpl(descs[i]);
        if (texture) results.push_back(std::move(texture));
    }
    return results;
}

std::vector<std::unique_ptr<IBuffer>> VulkanResourceFactory::CreateBuffersBatch(const BufferDesc* descs, uint32_t count) {
    std::vector<std::unique_ptr<IBuffer>> results;
    for (uint32_t i = 0; i < count; ++i) {
        auto buffer = CreateBufferImpl(descs[i]);
        if (buffer) results.push_back(std::move(buffer));
    }
    return results;
}

std::shared_ptr<Prisma::Graphic::IDescriptorSet> VulkanResourceFactory::CreateDescriptorSet(Prisma::Graphic::IDescriptorSetLayout* layout) {
    auto* vkLayout = dynamic_cast<VulkanDescriptorSetLayout*>(layout);
    if (!vkLayout || !m_device) return nullptr;

    VkDescriptorSetLayout layouts[] = { vkLayout->GetVkLayout() };
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_device->GetVkDescriptorPool();
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = layouts;

    VkDescriptorSet set;
    if (vkAllocateDescriptorSets(m_vkDevice, &allocInfo, &set) != VK_SUCCESS) {
        return nullptr;
    }

    return std::make_shared<VulkanDescriptorSet>(m_vkDevice, set);
}

std::shared_ptr<Prisma::Graphic::IDescriptorSetLayout> VulkanResourceFactory::CreateDescriptorSetLayout(const std::vector<Prisma::Graphic::ShaderResource>& resources) {
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    for (const auto& res : resources) {
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = res.Binding;
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_ALL;
        
        switch (res.ResourceType) {
            case Prisma::Graphic::ShaderResource::Type::Sampler2D:
            case Prisma::Graphic::ShaderResource::Type::SamplerCube:
            case Prisma::Graphic::ShaderResource::Type::Image2D:
                binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                break;
            case Prisma::Graphic::ShaderResource::Type::UniformBuffer:
                binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                break;
            default:
                continue;
        }
        bindings.push_back(binding);
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    VkDescriptorSetLayout layout;
    if (vkCreateDescriptorSetLayout(m_vkDevice, &layoutInfo, nullptr, &layout) != VK_SUCCESS) {
        return nullptr;
    }

    return std::make_shared<VulkanDescriptorSetLayout>(m_vkDevice, layout);
}

uint64_t VulkanResourceFactory::GetOrCreateTexturePool(TextureFormat format, uint32_t width, uint32_t height, uint32_t mipLevels, uint32_t arraySize) {
    for (const auto& [poolId, desc] : m_texturePoolDescs) {
        if (desc.format == format && desc.width == width && desc.height == height &&
            desc.mipLevels == mipLevels && desc.arraySize == arraySize) {
            return poolId;
        }
    }

    TextureDesc desc{};
    desc.width = width;
    desc.height = height;
    desc.depth = 1;
    desc.mipLevels = mipLevels == 0 ? 1 : mipLevels;
    desc.arraySize = arraySize == 0 ? 1 : arraySize;
    desc.format = format;
    desc.allowShaderResource = true;

    const uint64_t poolId = m_nextTexturePoolId++;
    m_texturePoolDescs.emplace(poolId, desc);
    m_texturePools.emplace(poolId, std::vector<std::unique_ptr<ITexture>>{});
    return poolId;
}

std::unique_ptr<ITexture> VulkanResourceFactory::AllocateFromTexturePool(uint64_t poolId) {
    auto poolIt = m_texturePools.find(poolId);
    if (poolIt == m_texturePools.end()) {
        return nullptr;
    }

    auto& pool = poolIt->second;
    if (!pool.empty()) {
        auto texture = std::move(pool.back());
        pool.pop_back();
        ++m_creationStats.texturesPooled;
        return texture;
    }

    auto descIt = m_texturePoolDescs.find(poolId);
    return descIt != m_texturePoolDescs.end() ? CreateTextureImpl(descIt->second) : nullptr;
}

void VulkanResourceFactory::DeallocateToTexturePool(uint64_t poolId, ITexture* texture) {
    if (!m_resourcePoolingEnabled || !texture) {
        return;
    }

    auto poolIt = m_texturePools.find(poolId);
    if (poolIt == m_texturePools.end()) {
        return;
    }

    LOG_DEBUG("Vulkan", "返回到池 {0} 的纹理是外部拥有的；调用者应通过池化句柄释放", poolId);
}
void VulkanResourceFactory::CleanupResourcePools() {}

bool VulkanResourceFactory::ValidateTextureDesc(const TextureDesc& desc, std::string& errorMsg) {
    if (desc.width == 0 || desc.height == 0) {
        errorMsg = "Texture dimensions must be greater than zero";
        return false;
    }
    if (desc.arraySize == 0) {
        errorMsg = "Texture array size must be greater than zero";
        return false;
    }
    if (desc.mipLevels == 0) {
        errorMsg = "Texture mip level count must be greater than zero";
        return false;
    }
    errorMsg.clear();
    return true;
}

bool VulkanResourceFactory::ValidateBufferDesc(const BufferDesc& desc, std::string& errorMsg) {
    if (desc.size == 0) {
        errorMsg = "Buffer size must be greater than zero";
        return false;
    }
    errorMsg.clear();
    return true;
}

bool VulkanResourceFactory::ValidateShaderDesc(const ShaderDesc& desc, std::string& errorMsg) {
    if (desc.entryPoint.empty()) {
        errorMsg = "Shader entry point is required";
        return false;
    }
    errorMsg.clear();
    return true;
}

void VulkanResourceFactory::GetMemoryBudget(uint64_t& budget, uint64_t& usage) const {
    budget = m_memoryLimit;
    usage = m_creationStats.totalMemoryAllocated;
}

void VulkanResourceFactory::SetMemoryLimit(uint64_t limit) { m_memoryLimit = limit; }
bool VulkanResourceFactory::IsMemoryLimitExceeded() const { return m_memoryLimit != 0 && m_creationStats.totalMemoryAllocated > m_memoryLimit; }
void VulkanResourceFactory::ForceGarbageCollection() { CleanupResourcePools(); }

IResourceFactory::ResourceCreationStats VulkanResourceFactory::GetCreationStats() const { return m_creationStats; }
void VulkanResourceFactory::ResetStats() { m_creationStats = {}; }

void VulkanResourceFactory::EnableResourcePooling(bool enable) { m_resourcePoolingEnabled = enable; }
void VulkanResourceFactory::SetPoolingThreshold(uint64_t threshold) { m_poolingThreshold = threshold; }
void VulkanResourceFactory::EnableDeferredDestruction(bool enable, uint32_t delayFrames) {
    m_deferredDestructionEnabled = enable;
    m_deferredDestructionDelayFrames = delayFrames;
}
void VulkanResourceFactory::ProcessDeferredDestructions() {
    if (!m_deferredDestructionEnabled || m_deferredDestructionDelayFrames == 0) {
        return;
    }
}

} // namespace Prisma::Graphic::Vulkan
