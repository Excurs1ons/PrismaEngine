#include "DX12Sampler.h"
#include "DX12RenderDevice.h"
#include <directx/d3dx12.h>
#include <d3d12.h>

namespace PrismaEngine::Graphic::DX12 {

DX12Sampler::DX12Sampler(DX12RenderDevice* device, const SamplerDesc& desc)
    : m_device(device)
    , m_desc(desc) {
}

DX12Sampler::~DX12Sampler() {
    // 资源由工厂管理
}

// ISampler接口实现
TextureFilter DX12Sampler::GetFilter() const {
    return m_desc.filter;
}

TextureAddressMode DX12Sampler::GetAddressU() const {
    return m_desc.addressU;
}

TextureAddressMode DX12Sampler::GetAddressV() const {
    return m_desc.addressV;
}

TextureAddressMode DX12Sampler::GetAddressW() const {
    return m_desc.addressW;
}

float DX12Sampler::GetMipLODBias() const {
    return m_desc.mipLODBias;
}

uint32_t DX12Sampler::GetMaxAnisotropy() const {
    return m_desc.maxAnisotropy;
}

TextureComparisonFunc DX12Sampler::GetComparisonFunc() const {
    return m_desc.comparisonFunc;
}

void DX12Sampler::GetBorderColor(float& r, float& g, float& b, float& a) const {
    r = m_desc.borderColor[0];
    g = m_desc.borderColor[1];
    b = m_desc.borderColor[2];
    a = m_desc.borderColor[3];
}

float DX12Sampler::GetMinLOD() const {
    return m_desc.minLOD;
}

float DX12Sampler::GetMaxLOD() const {
    return m_desc.maxLOD;
}

uint64_t DX12Sampler::GetHandle() const {
    return m_handle;
}

// DirectX12特定方法实现
D3D12_SAMPLER_DESC DX12Sampler::GetD3D12SamplerDesc() const {
    D3D12_SAMPLER_DESC desc = {};

    // 过滤模式转换
    switch (m_desc.filter) {
        case TextureFilter::Point:
            desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
            break;
        case TextureFilter::Linear:
            desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
            break;
        case TextureFilter::Anisotropic:
            desc.Filter = D3D12_FILTER_ANISOTROPIC;
            break;
        case TextureFilter::ComparisonPoint:
            desc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
            break;
        case TextureFilter::ComparisonLinear:
            desc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
            break;
        case TextureFilter::ComparisonAnisotropic:
            desc.Filter = D3D12_FILTER_COMPARISON_ANISOTROPIC;
            break;
        case TextureFilter::MinPointMagLinearMipPoint:
            desc.Filter = D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
            break;
        case TextureFilter::MinPointMagLinearMipLinear:
            desc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;  // 修正为正确的常量
            break;
        case TextureFilter::MinLinearMagPointMipPoint:
            desc.Filter = D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;  // 修正为正确的常量
            break;
        case TextureFilter::MinLinearMagPointMipLinear:
            desc.Filter = D3D12_FILTER_ANISOTROPIC;  // 使用各向异性过滤作为替代
            break;
        case TextureFilter::MinMagPointMipLinear:
            desc.Filter = D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
            break;
        case TextureFilter::MinLinearMagMipPoint:
            desc.Filter = D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
            break;
        default:
            desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
            break;
    }

    // 地址模式转换
    auto convertAddressMode = [](TextureAddressMode mode) {
        switch (mode) {
            case TextureAddressMode::Wrap: return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            case TextureAddressMode::Mirror: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
            case TextureAddressMode::Clamp: return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            case TextureAddressMode::Border: return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
            case TextureAddressMode::MirrorOnce: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
            default: return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        }
    };

    desc.AddressU = convertAddressMode(m_desc.addressU);
    desc.AddressV = convertAddressMode(m_desc.addressV);
    desc.AddressW = convertAddressMode(m_desc.addressW);

    // MIP设置
    desc.MipLODBias = m_desc.mipLODBias;
    desc.MaxAnisotropy = m_desc.maxAnisotropy;
    desc.MinLOD = m_desc.minLOD;
    desc.MaxLOD = m_desc.maxLOD;

    // 比较函数
    auto convertComparisonFunc = [](TextureComparisonFunc func) {
        switch (func) {
            case TextureComparisonFunc::Never: return D3D12_COMPARISON_FUNC_NEVER;
            case TextureComparisonFunc::Less: return D3D12_COMPARISON_FUNC_LESS;
            case TextureComparisonFunc::Equal: return D3D12_COMPARISON_FUNC_EQUAL;
            case TextureComparisonFunc::LessEqual: return D3D12_COMPARISON_FUNC_LESS_EQUAL;
            case TextureComparisonFunc::Greater: return D3D12_COMPARISON_FUNC_GREATER;
            case TextureComparisonFunc::NotEqual: return D3D12_COMPARISON_FUNC_NOT_EQUAL;
            case TextureComparisonFunc::GreaterEqual: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
            case TextureComparisonFunc::Always: return D3D12_COMPARISON_FUNC_ALWAYS;
            default: return D3D12_COMPARISON_FUNC_ALWAYS;
        }
    };

    desc.ComparisonFunc = convertComparisonFunc(m_desc.comparisonFunc);

    // 边框颜色
    desc.BorderColor[0] = m_desc.borderColor[0];
    desc.BorderColor[1] = m_desc.borderColor[1];
    desc.BorderColor[2] = m_desc.borderColor[2];
    desc.BorderColor[3] = m_desc.borderColor[3];

    return desc;
}

void DX12Sampler::CreateSampler(D3D12_CPU_DESCRIPTOR_HANDLE handle) {
    if (!m_device) {
        return;
    }

    auto d3d12Device = m_device->GetD3D12Device();
    if (!d3d12Device) {
        return;
    }

    auto samplerDesc = GetD3D12SamplerDesc();
    d3d12Device->CreateSampler(&samplerDesc, handle);

    m_handle = handle.ptr;
}

} // namespace PrismaEngine::Graphic::DX12