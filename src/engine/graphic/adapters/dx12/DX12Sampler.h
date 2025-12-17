#pragma once

#include "interfaces/ISampler.h"
#include <directx/d3dx12.h>
#include <d3d12.h>

namespace PrismaEngine::Graphic::DX12 {

// 前置声明
class DX12RenderDevice;

/// @brief DirectX12采样器适配器
/// 实现ISampler接口，包装采样器描述符
class DX12Sampler : public ISampler {
public:
    /// @brief 构造函数
    /// @param device DirectX12渲染设备
    /// @param desc 采样器描述
    /// @param handle 采样器句柄
    DX12Sampler(DX12RenderDevice* device, const SamplerDesc& desc, uint64_t handle);

    /// @brief 析构函数
    ~DX12Sampler() override;

    // ISampler接口实现
    TextureFilter GetFilter() const override;
    TextureAddressMode GetAddressU() const override;
    TextureAddressMode GetAddressV() const override;
    TextureAddressMode GetAddressW() const override;
    float GetMipLODBias() const override;
    uint32_t GetMaxAnisotropy() const override;
    TextureComparisonFunc GetComparisonFunc() const override;
    void GetBorderColor(float& r, float& g, float& b, float& a) const override;
    float GetMinLOD() const override;
    float GetMaxLOD() const override;
    uint64_t GetHandle() const override;

    // === DirectX12特定方法 ===

    /// @brief 创建D3D12采样器描述
    D3D12_SAMPLER_DESC GetD3D12SamplerDesc() const;

private:
    DX12RenderDevice* m_device;
    SamplerDesc m_desc;
    uint64_t m_handle;
};

} // namespace PrismaEngine::Graphic::DX12