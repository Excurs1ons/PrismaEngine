#pragma once

#include "interfaces/IRenderDevice.h"
#include "interfaces/ICommandBuffer.h"
#include "interfaces/IFence.h"
#include "interfaces/ISwapChain.h"
#include "interfaces/IResourceFactory.h"

// Vulkan headers
#include <vulkan/vulkan.h>
// #include <volk.h> // volk not available on Android

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <array>

namespace PrismaEngine::Graphic::Vulkan {

// 前置声明
class VulkanCommandBuffer;
class VulkanFence;
class VulkanSwapChain;
class VulkanResourceFactory;
class VulkanInstance;
class VulkanPhysicalDevice;
class VulkanLogicalDevice;
class VulkanQueue;

/// @brief Vulkan渲染设备
/// 实现IRenderDevice接口，基于Vulkan 1.3+
class RenderDeviceVulkan : public IRenderDevice {
public:
    /// @brief 构造函数
    RenderDeviceVulkan();

    /// @brief 析构函数
    ~RenderDeviceVulkan() override;

    // ========== IRenderDevice接口实现 ==========

    /// @brief 初始化设备
    bool Initialize(const DeviceDesc& desc) override;

    /// @brief 关闭设备
    void Shutdown() override;

    /// @brief 获取设备名称
    std::string GetName() const override;

    /// @brief 获取API名称
    std::string GetAPIName() const override;

    // ========== 命令缓冲区管理 ==========

    /// @brief 创建命令缓冲区
    std::unique_ptr<ICommandBuffer> CreateCommandBuffer(CommandBufferType type) override;

    /// @brief 提交命令缓冲区
    void SubmitCommandBuffer(ICommandBuffer* cmdBuffer, IFence* fence = nullptr) override;

    /// @brief 批量提交命令缓冲区
    void SubmitCommandBuffers(const std::vector<ICommandBuffer*>& cmdBuffers,
                             const std::vector<IFence*>& fences = {}) override;

    // ========== 同步操作 ==========

    /// @brief 等待设备空闲
    void WaitForIdle() override;

    /// @brief 创建围栏
    std::unique_ptr<IFence> CreateFence() override;

    /// @brief 等待围栏
    void WaitForFence(IFence* fence) override;

    // ========== 资源管理 ==========

    /// @brief 获取资源工厂
    IResourceFactory* GetResourceFactory() const override;

    // ========== 交换链管理 ==========

    /// @brief 创建交换链
    std::unique_ptr<ISwapChain> CreateSwapChain(void* windowHandle,
                                               uint32_t width,
                                               uint32_t height,
                                               bool vsync = true) override;

    /// @brief 获取当前交换链
    ISwapChain* GetSwapChain() const override;

    // ========== 帧管理 ==========

    /// @brief 开始帧
    void BeginFrame() override;

    /// @brief 结束帧
    void EndFrame() override;

    /// @brief 呈现
    void Present() override;

    // ========== 功能查询 ==========

    /// @brief 是否支持多线程
    bool SupportsMultiThreaded() const override;

    /// @brief 是否支持无绑定纹理
    bool SupportsBindlessTextures() const override;

    /// @brief 是否支持计算着色器
    bool SupportsComputeShader() const override;

    /// @brief 是否支持光线追踪
    bool SupportsRayTracing() const override;

    /// @brief 是否支持网格着色器
    bool SupportsMeshShader() const override;

    /// @brief 是否支持可变速率着色
    bool SupportsVariableRateShading() const override;

    // ========== 渲染统计 ==========

    /// @brief 获取GPU内存信息
    GPUMemoryInfo GetGPUMemoryInfo() const override;

    /// @brief 获取渲染统计
    RenderStats GetRenderStats() const override;

    // ========== 调试功能 ==========

    /// @brief 开始调试标记
    void BeginDebugMarker(const std::string& name) override;

    /// @brief 结束调试标记
    void EndDebugMarker() override;

    /// @brief 设置调试标记
    void SetDebugMarker(const std::string& name) override;

    // ========== Vulkan特定方法 ==========

    /// @brief 获取Vulkan实例
    VkInstance GetInstance() const;

    /// @brief 获取物理设备
    VkPhysicalDevice GetPhysicalDevice() const;

    /// @brief 获取逻辑设备
    VkDevice GetDevice() const;

    /// @brief 获取图形队列
    VulkanQueue* GetGraphicsQueue() const;

    /// @brief 获取计算队列
    VulkanQueue* GetComputeQueue() const;

    /// @brief 获取传输队列
    VulkanQueue* GetTransferQueue() const;

    /// @brief 获取命令池
    VkCommandPool GetCommandPool(CommandBufferType type) const;

    /// @brief 获取描述符集布局缓存
    VkDescriptorSetLayout GetDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings);

    /// @brief 获取管线布局缓存
    VkPipelineLayout GetPipelineLayout(VkDescriptorSetLayout descriptorSetLayout,
                                       const std::vector<VkPushConstantRange>& pushConstants = {});

    /// @brief 获取采样器
    VkSampler GetSampler(VkFilter minFilter, VkFilter magFilter,
                         VkSamplerAddressMode addressMode);

    /// @brief 查找内存类型
    /// @param typeFilter 类型过滤器
    /// @param properties 内存属性
    /// @return 内存类型索引，失败返回UINT32_MAX
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

    /// @brief 创建缓冲区
    VkBuffer CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage);

    /// @brief 分配内存
    VkDeviceMemory AllocateMemory(VkDeviceSize size, uint32_t memoryTypeIndex);

    /// @brief 绑定缓冲区内存
    void BindBufferMemory(VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize offset = 0);

    /// @brief 映射内存
    void* MapMemory(VkDeviceMemory memory, VkDeviceSize size, VkDeviceSize offset = 0);

    /// @brief 取消映射内存
    void UnmapMemory(VkDeviceMemory memory);

    /// @brief 创建图像
    VkImage CreateImage(uint32_t width, uint32_t height, uint32_t depth,
                        VkFormat format, VkImageUsageFlags usage,
                        uint32_t mipLevels = 1, uint32_t arrayLayers = 1);

    /// @brief 创建图像视图
    VkImageView CreateImageView(VkImage image, VkFormat format,
                               VkImageViewType viewType, VkImageAspectFlags aspectFlags,
                               uint32_t mipLevels = 1, uint32_t baseMipLevel = 0,
                               uint32_t layerCount = 1, uint32_t baseLayer = 0);

private:
    // ========== 初始化相关 ==========

    /// @brief 初始化Vulkan实例
    bool InitializeInstance();

    /// @brief 选择物理设备
    bool SelectPhysicalDevice();

    /// @brief 创建逻辑设备
    bool CreateLogicalDevice();

    /// @brief 创建命令池
    bool CreateCommandPools();

    /// @brief 初始化调试
    bool InitializeDebug();

    /// @brief 创建同步对象
    bool CreateSynchronizationObjects();

    /// @brief 释放所有资源
    void ReleaseAll();

    // ========== 验证层和扩展 ==========

    /// @brief 检查验证层支持
    bool CheckValidationLayerSupport();

    /// @brief 获取所需扩展
    std::vector<const char*> GetRequiredExtensions();

    /// @brief 查找设备扩展支持
    bool IsDeviceExtensionSupported(const std::string& extension);

    // ========== 队列族 ==========

    /// @brief 查找队列族
    struct QueueFamilyIndices {
        uint32_t graphics = UINT32_MAX;
        uint32_t compute = UINT32_MAX;
        uint32_t transfer = UINT32_MAX;
        uint32_t present = UINT32_MAX;

        bool IsComplete() const {
            return graphics != UINT32_MAX &&
                   compute != UINT32_MAX &&
                   transfer != UINT32_MAX &&
                   present != UINT32_MAX;
        }
    };

    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physicalDevice);

    // ========== 缓存管理 ==========

    /// @brief 创建描述符集布局缓存
    struct DescriptorSetLayoutCacheKey {
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        bool operator==(const DescriptorSetLayoutCacheKey& other) const {
            if (bindings.size() != other.bindings.size()) return false;
            for (size_t i = 0; i < bindings.size(); ++i) {
                if (memcmp(&bindings[i], &other.bindings[i], sizeof(VkDescriptorSetLayoutBinding)) != 0) {
                    return false;
                }
            }
            return true;
        }
    };

    struct DescriptorSetLayoutCacheKeyHash {
        size_t operator()(const DescriptorSetLayoutCacheKey& key) const {
            size_t hash = 0;
            for (const auto& binding : key.bindings) {
                hash ^= std::hash<uint32_t>()(static_cast<uint32_t>(binding.binding));
                hash ^= std::hash<uint32_t>()(static_cast<uint32_t>(binding.descriptorType));
                hash ^= std::hash<uint32_t>()(static_cast<uint32_t>(binding.descriptorCount));
                hash ^= std::hash<uint32_t>()(static_cast<uint32_t>(binding.stageFlags));
            }
            return hash;
        }
    };

    // ========== 成员变量 ==========

    // Vulkan核心组件
    std::unique_ptr<VulkanInstance> m_instance;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    std::unique_ptr<VulkanLogicalDevice> m_device;

    // 队列
    std::unique_ptr<VulkanQueue> m_graphicsQueue;
    std::unique_ptr<VulkanQueue> m_computeQueue;
    std::unique_ptr<VulkanQueue> m_transferQueue;
    std::unique_ptr<VulkanQueue> m_presentQueue;

    // 命令池
    VkCommandPool m_graphicsCommandPool = VK_NULL_HANDLE;
    VkCommandPool m_computeCommandPool = VK_NULL_HANDLE;
    VkCommandPool m_transferCommandPool = VK_NULL_HANDLE;

    // 交换链
    std::unique_ptr<VulkanSwapChain> m_swapChain;

    // 资源工厂
    std::unique_ptr<VulkanResourceFactory> m_resourceFactory;

    // 调试
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    bool m_debugEnabled = false;

    // 缓存
    std::unordered_map<DescriptorSetLayoutCacheKey, VkDescriptorSetLayout,
                       DescriptorSetLayoutCacheKeyHash> m_descriptorSetLayoutCache;
    std::unordered_map<size_t, VkPipelineLayout> m_pipelineLayoutCache;
    std::unordered_map<uint64_t, VkSampler> m_samplerCache;

    // 同步对象
    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;

    // 设备能力
    struct DeviceFeatures {
        VkPhysicalDeviceFeatures features{};
        VkPhysicalDeviceProperties properties{};
        VkPhysicalDeviceMemoryProperties memoryProperties{};

        // 扩展功能
        bool supportsBindless = false;
        bool supportsRayTracing = false;
        bool supportsMeshShading = false;
        bool supportsVariableRateShading = false;
        bool supportsFragmentDensityMaps = false;
        bool supportsShaderFloat16 = false;
        bool supportsShaderInt8 = false;
    } m_deviceFeatures;

    // 队列族索引
    QueueFamilyIndices m_queueFamilies;

    // 渲染统计
    RenderStats m_stats;

    // 配置
    DeviceDesc m_desc;

    // 当前帧索引
    uint32_t m_currentFrame = 0;
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;

    // 状态
    bool m_initialized = false;
};

} // namespace PrismaEngine::Graphic::Vulkan