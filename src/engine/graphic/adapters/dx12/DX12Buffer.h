#pragma once

#include "interfaces/IBuffer.h"
#include "interfaces/IResourceManager.h"
#include <directx/d3dx12.h>
#include <d3d12.h>
#include <memory>

namespace PrismaEngine::Graphic::DX12 {

// 前置声明
class DX12RenderDevice;

/// @brief DirectX12缓冲区适配器
/// 实现IBuffer接口，包装ID3D12Resource
class DX12Buffer : public IBuffer {
public:
    /// @brief 构造函数
    /// @param device DirectX12渲染设备
    /// @param resource D3D12资源
    /// @param desc 缓冲区描述
    DX12Buffer(DX12RenderDevice* device,
               Microsoft::WRL::ComPtr<ID3D12Resource> resource,
               const BufferDesc& desc);

    /// @brief 析构函数
    ~DX12Buffer() override;

    ResourceType GetType() const override;
    // IBuffer接口实现
    BufferType GetBufferType() const override;
    uint64_t GetSize() const override;
    uint32_t GetStride() const override;
    BufferUsage GetUsage() const override;
    uint32_t GetElementCount() const override;

    bool IsDynamic() const override;
    bool IsReadOnly() const override;
    bool IsShaderResource() const override;
    bool IsUnorderedAccess() const override;

    BufferMapDesc Map(uint64_t offset, uint64_t size, uint32_t mapType) override;
    void Unmap(uint64_t offset, uint64_t size) override;

    void UpdateData(const void* data, uint64_t size, uint64_t offset) override;
    bool ReadData(void* dstBuffer, uint64_t size, uint64_t offset) override;

    void CopyTo(IBuffer* dstBuffer,
                uint64_t srcOffset, uint64_t dstOffset, uint64_t size) override;

    void Fill(uint32_t value, uint64_t offset, uint64_t size) override;

    void CopyFromTexture(ITexture* srcTexture,
                        uint32_t srcMipLevel, uint32_t srcArraySlice) override;

    void CopyToTexture(ITexture* dstTexture,
                      uint32_t dstMipLevel, uint32_t dstArraySlice) override;

    uint64_t CreateView(BufferDescriptorType descType, const BufferViewDesc& desc) override;

    uint64_t GetDefaultSRV() const override;
    uint64_t GetDefaultUAV() const override;
    uint64_t GetDefaultCBV() const override;
    uint64_t GetDefaultVBV() const override;
    uint64_t GetDefaultIBV() const override;

    uint64_t AllocateDynamic(uint64_t size, uint64_t alignment) override;
    void ResetDynamicAllocation() override;
    uint64_t GetCurrentDynamicOffset() const override;
    uint64_t GetAvailableDynamicSpace() const override;

    bool DebugSaveToFile(const std::string& filename,
                        const std::string& format,
                        uint64_t offset, uint64_t size) override;

    bool DebugValidateContent(const void* expectedData,
                             uint64_t size, uint64_t offset) override;

    void DebugPrintInfo() const override;

    void Discard(uint64_t offset, uint64_t size) override;
    void Reserve(uint64_t size) override;
    void Compact() override;

    uint64_t GetMemoryUsage() const override;
    uint64_t GetGPUMemoryUsage() const override;

    // === DirectX12特定方法 ===

    /// @brief 获取D3D12资源
    /// @return 资源指针
    ID3D12Resource* GetResource() const { return m_resource.Get(); }

    /// @brief 获取GPU地址
    /// @return GPU虚拟地址
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const {
        return m_resource->GetGPUVirtualAddress();
    }

    /// @brief 获取CPU地址（对于上传缓冲区）
    /// @return CPU映射地址
    void* GetCPUAddress() const { return m_cpuAddress; }

    /// @brief 获取当前动态偏移量
    /// @return 偏移量
    uint64_t GetDynamicOffset() const { return m_dynamicOffset; }

    /// @brief 设置动态偏移量
    /// @param offset 偏移量
    void SetDynamicOffset(uint64_t offset) { m_dynamicOffset = offset; }

    /// @brief 创建CBV描述符
    void CreateCBV(D3D12_CPU_DESCRIPTOR_HANDLE handle, uint32_t size, uint64_t offset);

    /// @brief 创建SRV描述符
    void CreateSRV(D3D12_CPU_DESCRIPTOR_HANDLE handle, const BufferViewDesc& desc);

    /// @brief 创建UAV描述符
    void CreateUAV(D3D12_CPU_DESCRIPTOR_HANDLE handle, const BufferViewDesc& desc);

    /// @brief 获取堆标志
    D3D12_HEAP_FLAGS GetHeapFlags() const;

    /// @brief 获取初始资源状态
    D3D12_RESOURCE_STATES GetInitialResourceState() const;

private:
    DX12RenderDevice* m_device;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;
    BufferDesc m_desc;

    // CPU映射地址（用于上传缓冲区）
    void* m_cpuAddress = nullptr;

    // 动态分配管理
    uint64_t m_dynamicOffset = 0;
    uint64_t m_dynamicSize = 0;

    // 描述符句柄
    D3D12_GPU_DESCRIPTOR_HANDLE m_srv = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_uav = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_cbv = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_vbv = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_ibv = {};

    // 状态跟踪
    bool m_mapped = false;

    // 缓冲区步长
    uint32_t m_stride = 0;

    // 辅助方法
    D3D12_RESOURCE_DESC GetD3D12ResourceDesc() const;
    D3D12_RESOURCE_STATES GetInitialState() const;
    D3D12_HEAP_TYPE GetHeapType() const;
    bool IsUploadHeap() const;
    bool IsReadbackHeap() const;
    bool IsDefaultHeap() const;

public:
    ResourceId GetId() const override;
    const std::string& GetName() const override;
    void SetName(const std::string& name) override;
    bool IsLoaded() const override;
    bool IsValid() const override;
    void AddRef() override;
    uint32_t Release() override;
    uint32_t GetRefCount() const override;
    const std::string& GetDebugTag() const override;
    void SetDebugTag(const std::string& tag) override;
    uint64_t GetCreationTimestamp() const override;
    uint64_t GetLastAccessTimestamp() const override;
    void MarkDirty() override;
    bool IsDirty() const override;
    void ClearDirty() override;
};

} // namespace PrismaEngine::Graphic::DX12