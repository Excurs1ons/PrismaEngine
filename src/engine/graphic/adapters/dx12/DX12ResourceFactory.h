#pragma once

#include "interfaces/IResourceFactory.h"
#include <directx/d3d12.h>
#include <memory>
#include <unordered_map>
#include <vector>
#include <wrl/client.h>

namespace PrismaEngine::Graphic::DX12 {

// 前置声明
class DX12RenderDevice;
class DX12Texture;
class DX12Buffer;
class DX12Shader;
class DX12Pipeline;
class DX12Sampler;

/// @brief DirectX12资源工厂适配器
/// 实现IResourceFactory接口，创建DirectX12特定的资源
class DX12ResourceFactory : public IResourceFactory {
public:
    /// @brief 构造函数
    /// @param device DirectX12渲染设备
    explicit DX12ResourceFactory(DX12RenderDevice* device);

    /// @brief 析构函数
    ~DX12ResourceFactory() override;

    // IResourceFactory接口实现
    bool Initialize(IRenderDevice* device) override;
    void Shutdown() override;

    std::unique_ptr<ITexture> CreateTextureImpl(const TextureDesc& desc) override;
    std::unique_ptr<ITexture> CreateTextureFromFile(const std::string& filename,
                                                   const TextureDesc* desc) override;
    std::unique_ptr<ITexture> CreateTextureFromMemory(const void* data,
                                                    uint64_t dataSize,
                                                    const TextureDesc& desc) override;

    std::unique_ptr<IBuffer> CreateBufferImpl(const BufferDesc& desc) override;
    std::unique_ptr<IBuffer> CreateDynamicBuffer(uint64_t size,
                                                BufferType type,
                                                BufferUsage usage) override;

    std::unique_ptr<IShader> CreateShaderImpl(const ShaderDesc& desc,
                                             const std::vector<uint8_t>& bytecode,
                                             const ShaderReflection& reflection) override;

    std::unique_ptr<IPipelineState> CreatePipelineStateImpl() override;
    std::unique_ptr<ISampler> CreateSamplerImpl(const SamplerDesc& desc) override;

    std::unique_ptr<ISwapChain> CreateSwapChainImpl(void* windowHandle,
                                                    uint32_t width,
                                                    uint32_t height,
                                                    TextureFormat format,
                                                    uint32_t bufferCount,
                                                    bool vsync) override;

    std::unique_ptr<IFence> CreateFenceImpl() override;

    std::vector<std::unique_ptr<ITexture>> CreateTexturesBatch(const TextureDesc* descs,
                                                             uint32_t count) override;
    std::vector<std::unique_ptr<IBuffer>> CreateBuffersBatch(const BufferDesc* descs,
                                                           uint32_t count) override;

    uint64_t GetOrCreateTexturePool(TextureFormat format,
                                    uint32_t width,
                                    uint32_t height,
                                    uint32_t mipLevels,
                                    uint32_t arraySize) override;
    std::unique_ptr<ITexture> AllocateFromTexturePool(uint64_t poolId) override;
    void DeallocateToTexturePool(uint64_t poolId, ITexture* texture) override;
    void CleanupResourcePools() override;

    bool ValidateTextureDesc(const TextureDesc& desc, std::string& errorMsg) override;
    bool ValidateBufferDesc(const BufferDesc& desc, std::string& errorMsg) override;
    bool ValidateShaderDesc(const ShaderDesc& desc, std::string& errorMsg) override;
    bool ValidatePipelineDesc(const PipelineDesc& desc, std::string& errorMsg);

    void GetMemoryBudget(uint64_t& budget, uint64_t& usage) const override;
    void SetMemoryLimit(uint64_t limit) override;
    bool IsMemoryLimitExceeded() const override;
    void ForceGarbageCollection() override;

    ResourceCreationStats GetCreationStats() const override;
    void ResetStats() override;

    void EnableResourcePooling(bool enable) override;
    void SetPoolingThreshold(uint64_t threshold) override;
    void EnableDeferredDestruction(bool enable, uint32_t delayFrames) override;
    void ProcessDeferredDestructions() override;

    // === DirectX12特定方法 ===

    /// @brief 编译着色器
    /// @param desc 着色器描述
    /// @param[out] bytecode 编译后的字节码
    /// @param[out] reflection 反射信息
    /// @param[out] errors 错误信息
    /// @return 是否编译成功
    bool CompileShader(const ShaderDesc& desc,
                      std::vector<uint8_t>& bytecode,
                      ShaderReflection& reflection,
                      std::string& errors);

    /// @brief 创建描述符堆
    /// @param type 堆类型
    /// @param numDescriptors 描述符数量
    /// @param flags 堆标志
    /// @return 堆指针
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(
        D3D12_DESCRIPTOR_HEAP_TYPE type,
        uint32_t numDescriptors,
        D3D12_DESCRIPTOR_HEAP_FLAGS flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

    /// @brief 分配描述符
    /// @param heap 堆指针
    /// @param[out] cpuHandle CPU句柄
    /// @param[out] gpuHandle GPU句柄（如果堆可见）
    /// @return 描述符索引
    uint32_t AllocateDescriptor(ID3D12DescriptorHeap* heap,
                                D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle,
                                D3D12_GPU_DESCRIPTOR_HANDLE& gpuHandle);

    /// @brief 获取着色器模型
    /// @param target 编译目标
    /// @return 着色器模型
    std::string GetShaderModel(const std::string& target) const;

private:
    DX12RenderDevice* m_device;
    bool m_initialized = false;

    // 描述符堆管理
    struct DescriptorHeap {
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;
        D3D12_DESCRIPTOR_HEAP_TYPE type;
        uint32_t descriptorSize;
        uint32_t capacity;
        uint32_t usedCount;
        D3D12_CPU_DESCRIPTOR_HANDLE cpuStart;
        D3D12_GPU_DESCRIPTOR_HANDLE gpuStart;
    };

    std::unordered_map<D3D12_DESCRIPTOR_HEAP_TYPE, std::unique_ptr<DescriptorHeap>> m_descriptorHeaps;

    // 资源池
    struct TexturePool {
        TextureFormat format;
        uint32_t width;
        uint32_t height;
        uint32_t mipLevels;
        uint32_t arraySize;
        std::vector<std::unique_ptr<DX12Texture>> freeTextures;
        uint64_t totalAllocated = 0;
        uint64_t peakUsage = 0;
    };

    std::unordered_map<uint64_t, std::unique_ptr<TexturePool>> m_texturePools;
    uint64_t m_nextPoolId = 1;

    // 配置选项
    bool m_resourcePoolingEnabled = true;
    uint64_t m_poolingThreshold = 1024 * 1024; // 1MB
    bool m_deferredDestructionEnabled = true;
    uint32_t m_destructionDelayFrames = 2;
    uint64_t m_memoryLimit = 0; // 0 = 无限制

    // 延迟销毁队列
    struct DeferredResource {
        std::unique_ptr<ITexture> texture;
        std::unique_ptr<IBuffer> buffer;
        uint32_t framesRemaining;
    };

    std::vector<DeferredResource> m_deferredResources;

    // 统计信息
    mutable ResourceCreationStats m_stats = {};

    // 辅助方法
    D3D12_RESOURCE_DESC GetD3D12TextureDesc(const TextureDesc& desc) const;
    D3D12_RESOURCE_DESC GetD3D12BufferDesc(const BufferDesc& desc) const;
    D3D12_HEAP_TYPE GetHeapType(BufferUsage usage) const;
    D3D12_HEAP_FLAGS GetHeapFlags(BufferUsage usage) const;
    D3D12_RESOURCE_STATES GetInitialResourceState(BufferType type, BufferUsage usage) const;

    Microsoft::WRL::ComPtr<ID3D12Resource> CreateCommittedResource(
        const D3D12_RESOURCE_DESC& desc,
        D3D12_HEAP_TYPE heapType,
        D3D12_HEAP_FLAGS heapFlags,
        D3D12_RESOURCE_STATES initialState);

    bool LoadImageFromFile(const std::string& filename,
                           std::vector<uint8_t>& imageData,
                           TextureDesc& desc);

    uint64_t CalculateTexturePoolKey(const TextureDesc& desc) const;
    void InitializeDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t initialCapacity);

    /// @brief Convert texture format to DXGI format
    DXGI_FORMAT GetDXGIFormat(TextureFormat format) const;
};

} // namespace PrismaEngine::Graphic::DX12