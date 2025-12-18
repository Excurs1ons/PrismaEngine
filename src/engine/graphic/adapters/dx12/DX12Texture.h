#pragma once

#include "interfaces/IResourceManager.h"
#include "interfaces/ITexture.h"
#include <directx/d3dx12.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <memory>

namespace PrismaEngine::Graphic::DX12 {

// 前置声明
class DX12RenderDevice;

/// @brief DirectX12纹理适配器
/// 实现ITexture接口，包装ID3D12Resource
class DX12Texture : public ITexture {
public:
    /// @brief 构造函数
    /// @param device DirectX12渲染设备
    /// @param resource D3D12资源
    /// @param desc 纹理描述
    DX12Texture(DX12RenderDevice* device,
                Microsoft::WRL::ComPtr<ID3D12Resource> resource,
                const TextureDesc& desc);

    /// @brief 析构函数
    ~DX12Texture() override;

    // IResource接口实现
    ResourceType GetType() const override;
    ResourceId GetId() const override;
    const std::string& GetName() const override;
    void SetName(const std::string& name) override;
    uint64_t GetSize() const override;
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

    // ITexture接口实现
    TextureType GetTextureType() const override;
    TextureFormat GetFormat() const override;
    uint32_t GetWidth() const override;
    uint32_t GetHeight() const override;
    uint32_t GetDepth() const override;
    uint32_t GetMipLevels() const override;
    uint32_t GetArraySize() const override;
    uint32_t GetSampleCount() const override;
    uint32_t GetSampleQuality() const override;

    bool IsRenderTarget() const override;
    bool IsDepthStencil() const override;
    bool IsShaderResource() const override;
    bool IsUnorderedAccess() const override;

    uint32_t GetBytesPerPixel() const override;
    uint64_t GetSubresourceSize(uint32_t mipLevel) const override;

    TextureMapDesc Map(uint32_t mipLevel, uint32_t arraySlice, uint32_t mapType) override;
    void Unmap(uint32_t mipLevel, uint32_t arraySlice) override;

    void UpdateData(const void* data, uint64_t dataSize,
                   uint32_t mipLevel, uint32_t arraySlice,
                   uint32_t left, uint32_t top, uint32_t front,
                   uint32_t width, uint32_t height, uint32_t depth) override;

    void GenerateMips() override;

    void CopyFrom(ITexture* srcTexture,
                 uint32_t srcMipLevel, uint32_t srcArraySlice,
                 uint32_t dstMipLevel, uint32_t dstArraySlice) override;

    bool ReadData(uint32_t mipLevel, uint32_t arraySlice,
                 void* dstBuffer, uint64_t bufferSize) override;

    uint64_t CreateDescriptor(TextureDescriptorType descType,
                             TextureFormat format,
                             uint32_t mipLevel,
                             uint32_t arraySize) override;

    uint64_t GetDefaultSRV() const override;
    uint64_t GetDefaultRTV() const override;
    uint64_t GetDefaultDSV() const override;
    uint64_t GetDefaultUAV() const override;

    void Clear(const Color& color, uint32_t mipLevel, uint32_t arraySlice) override;
    void ClearDepthStencil(float depth, uint8_t stencil) override;
    void ResolveMultisampled(ITexture* dstTexture, TextureFormat format) override;

    void Discard(uint32_t mipLevel, uint32_t arraySlice) override;
    void Compact() override;
    uint64_t GetMemoryUsage() const override;

    bool DebugSaveToFile(const std::string& filename,
                        uint32_t mipLevel, uint32_t arraySlice) override;
    bool Validate() override;

    // === DirectX12特定方法 ===

    /// @brief 获取D3D12资源
    /// @return 资源指针
    ID3D12Resource* GetResource() const { return m_resource.Get(); }

    /// @brief 获取RTV句柄
    /// @return RTV句柄
    D3D12_CPU_DESCRIPTOR_HANDLE GetRTV() const { return m_rtv; }

    /// @brief 获取DSV句柄
    /// @return DSV句柄
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() const { return m_dsv; }

    /// @brief 获取SRV句柄
    /// @return SRV句柄
    D3D12_GPU_DESCRIPTOR_HANDLE GetSRV() const { return m_srv; }

    /// @brief 获取UAV句柄
    /// @return UAV句柄
    D3D12_GPU_DESCRIPTOR_HANDLE GetUAV() const { return m_uav; }

    /// @brief 创建RTV描述符
    void CreateRTV(D3D12_CPU_DESCRIPTOR_HANDLE handle);

    /// @brief 创建DSV描述符
    void CreateDSV(D3D12_CPU_DESCRIPTOR_HANDLE handle);

    /// @brief 创建SRV描述符
    void CreateSRV(D3D12_CPU_DESCRIPTOR_HANDLE handle);

    /// @brief 创建UAV描述符
    void CreateUAV(D3D12_CPU_DESCRIPTOR_HANDLE handle);

private:
    DX12RenderDevice* m_device;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;
    TextureDesc m_desc;

    // 描述符句柄
    D3D12_CPU_DESCRIPTOR_HANDLE m_rtv = {};
    D3D12_CPU_DESCRIPTOR_HANDLE m_dsv = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_srv = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_uav = {};

    // IResource接口数据
    std::string m_name;
    std::string m_debugTag;
    uint32_t m_refCount = 1;
    uint64_t m_creationTimestamp = 0;
    uint64_t m_lastAccessTimestamp = 0;
    bool m_isDirty = false;

    // 状态跟踪
    bool m_mapped = false;
    void* m_mappedData = nullptr;

    // 辅助方法
    D3D12_RESOURCE_DESC GetD3D12ResourceDesc() const;
    DXGI_FORMAT GetDXGIFormat() const;
    D3D12_RESOURCE_STATES GetInitialState() const;
    D3D12_RESOURCE_STATES GetCurrentState() const { return m_currentState; }
    void SetCurrentState(D3D12_RESOURCE_STATES state) { m_currentState = state; }

    D3D12_RESOURCE_STATES m_currentState = D3D12_RESOURCE_STATE_COMMON;
};

} // namespace PrismaEngine::Graphic::DX12