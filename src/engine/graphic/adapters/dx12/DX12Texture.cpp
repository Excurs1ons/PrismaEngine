#include "DX12Texture.h"
#include "DX12RenderDevice.h"
#include <dxgi1_6.h>
#include <directx/d3dx12.h>
#include <wrl/client.h>
#include <sstream>

using Microsoft::WRL::ComPtr;

namespace PrismaEngine::Graphic::DX12 {

DX12Texture::DX12Texture(DX12RenderDevice* device,
                         ComPtr<ID3D12Resource> resource,
                         const TextureDesc& desc)
    : m_device(device)
    , m_resource(resource)
    , m_desc(desc)
    , m_currentState(D3D12_RESOURCE_STATE_COMMON) {

    // 从资源获取实际属性
    if (resource) {
        auto desc = resource->GetDesc();
        m_desc.width = desc.Width;
        m_desc.height = desc.Height;
        m_desc.depth = desc.DepthOrArraySize;
        m_desc.mipLevels = desc.MipLevels;
        m_desc.arraySize = desc.DepthOrArraySize;
        m_desc.format = TextureFormat::RGBA8_UNorm; // 需要根据DXGI格式转换

        // 判断纹理类型
        if (desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE1D) {
            m_desc.type = TextureType::Texture1D;
        } else if (desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D) {
            if (desc.DepthOrArraySize > 1) {
                m_desc.type = TextureType::Texture2DArray;
            } else {
                m_desc.type = TextureType::Texture2D;
            }
        } else if (desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D) {
            m_desc.type = TextureType::Texture3D;
        }
    }

    // 设置资源ID
    m_id = static_cast<ResourceId>(reinterpret_cast<uintptr_t>(resource.Get()));
}

DX12Texture::~DX12Texture() {
    // 资源由ComPtr自动管理
}

// ITexture接口实现
TextureType DX12Texture::GetTextureType() const {
    return m_desc.type;
}

TextureFormat DX12Texture::GetFormat() const {
    return m_desc.format;
}

uint32_t DX12Texture::GetWidth() const {
    return m_desc.width;
}

uint32_t DX12Texture::GetHeight() const {
    return m_desc.height;
}

uint32_t DX12Texture::GetDepth() const {
    return m_desc.depth;
}

uint32_t DX12Texture::GetMipLevels() const {
    return m_desc.mipLevels;
}

uint32_t DX12Texture::GetArraySize() const {
    return m_desc.arraySize;
}

uint32_t DX12Texture::GetSampleCount() const {
    if (m_resource) {
        auto desc = m_resource->GetDesc();
        return desc.SampleDesc.Count;
    }
    return 1;
}

uint32_t DX12Texture::GetSampleQuality() const {
    if (m_resource) {
        auto desc = m_resource->GetDesc();
        return desc.SampleDesc.Quality;
    }
    return 0;
}

bool DX12Texture::IsRenderTarget() const {
    return m_desc.allowRenderTarget;
}

bool DX12Texture::IsDepthStencil() const {
    return m_desc.format == TextureFormat::D32_Float ||
           m_desc.format == TextureFormat::D24_UNorm_S8_UInt ||
           m_desc.format == TextureFormat::D32_Float_S8_UInt;
}

bool DX12Texture::IsShaderResource() const {
    return m_desc.allowShaderResource;
}

bool DX12Texture::IsUnorderedAccess() const {
    return m_desc.allowUnorderedAccess;
}

uint32_t DX12Texture::GetBytesPerPixel() const {
    // 根据格式计算每像素字节数
    switch (m_desc.format) {
        case TextureFormat::R8_UNorm:
        case TextureFormat::R8_SNorm:
        case TextureFormat::R8_UInt:
        case TextureFormat::R8_SInt:
            return 1;
        case TextureFormat::R16_UNorm:
        case TextureFormat::R16_SNorm:
        case TextureFormat::R16_Float:
        case TextureFormat::R16_UInt:
        case TextureFormat::R16_SInt:
        case TextureFormat::RG16_UNorm:
        case TextureFormat::RG16_SNorm:
        case TextureFormat::RG16_Float:
        case TextureFormat::RG16_UInt:
        case TextureFormat::RG16_SInt:
            return 2;
        case TextureFormat::R32_Float:
        case TextureFormat::R32_UInt:
        case TextureFormat::R32_SInt:
            return 4;
        case TextureFormat::RGBA8_UNorm:
        case TextureFormat::RGBA8_SNorm:
        case TextureFormat::RGBA8_UInt:
        case TextureFormat::RGBA8_SInt:
            return 4;
        case TextureFormat::RGB32_Float:
        case TextureFormat::RGB32_UInt:
        case TextureFormat::RGB32_SInt:
            return 12;
        case TextureFormat::RGBA32_Float:
        case TextureFormat::RGBA32_UInt:
        case TextureFormat::RGBA32_SInt:
            return 16;
        default:
            return 4;  // 默认假设4字节
    }
}

uint64_t DX12Texture::GetSubresourceSize(uint32_t mipLevel) const {
    uint32_t width = m_desc.width >> mipLevel;
    uint32_t height = m_desc.height >> mipLevel;
    uint32_t depth = m_desc.depth >> mipLevel;

    uint32_t bytesPerPixel = GetBytesPerPixel();

    return width * height * depth * bytesPerPixel;
}

TextureMapDesc DX12Texture::Map(uint32_t mipLevel, uint32_t arraySlice, uint32_t mapType) {
    if (!m_resource || m_mapped) {
        return {};
    }

    // 检查资源是否可以映射
    auto desc = m_resource->GetDesc();
    if (desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D &&
        (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET ||
         desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)) {
        // 渲染目标和UAV纹理不能映射
        return {};
    }

    D3D12_RANGE readRange = {0, 0};
    if (mapType == 0) {  // Read
        readRange.End = GetSubresourceSize(mipLevel);
    }

    // 计算子资源索引
    uint32_t subresource = mipLevel + arraySlice * m_desc.mipLevels;

    // 映射资源
    void* data = nullptr;
    HRESULT hr = m_resource->Map(subresource, &readRange, &data);

    if (FAILED(hr)) {
        return {};
    }

    m_mapped = true;

    TextureMapDesc mapDesc;
    mapDesc.data = data;
    mapDesc.size = GetSubresourceSize(mipLevel);
    mapDesc.offset = 0;

    return mapDesc;
}

void DX12Texture::Unmap(uint32_t mipLevel, uint32_t arraySlice) {
    if (!m_resource || !m_mapped) {
        return;
    }

    uint32_t subresource = mipLevel + arraySlice * m_desc.mipLevels;
    m_resource->Unmap(subresource, nullptr);

    m_mapped = false;
}

void DX12Texture::UpdateData(const void* data, uint64_t dataSize,
                           uint32_t mipLevel, uint32_t arraySlice,
                           uint32_t left, uint32_t top, uint32_t front,
                           uint32_t width, uint32_t height, uint32_t depth) {
    // 使用命令缓冲区更新纹理
    // 这需要通过DX12RenderDevice的命令缓冲区来完成
    // 这里暂时不实现
}

void DX12Texture::GenerateMips() {
    // 生成MIP映射
    // 需要使用DX12的生成MIP映射功能
    // 这里暂时不实现
}

void DX12Texture::CopyFrom(ITexture* srcTexture,
                         uint32_t srcMipLevel, uint32_t srcArraySlice,
                         uint32_t dstMipLevel, uint32_t dstArraySlice) {
    auto srcDX12 = static_cast<DX12Texture*>(srcTexture);
    if (!srcDX12 || !srcDX12->m_resource || !m_resource) {
        return;
    }

    // 需要通过命令缓冲区执行复制
    // 这里暂时不实现
}

bool DX12Texture::ReadData(uint32_t mipLevel, uint32_t arraySlice,
                           void* dstBuffer, uint64_t bufferSize) {
    if (!m_resource) {
        return false;
    }

    // 映射资源并读取数据
    auto mapDesc = Map(mipLevel, arraySlice, 1);
    if (!mapDesc.data) {
        return false;
    }

    uint64_t size = std::min(mapDesc.size, bufferSize);
    memcpy(dstBuffer, mapDesc.data, size);

    Unmap(mipLevel, arraySlice);
    return true;
}

uint64_t DX12Texture::CreateDescriptor(TextureDescriptorType descType,
                                     TextureFormat format,
                                     uint32_t mipLevel,
                                     uint32_t arraySize) {
    // 需要通过DX12ResourceFactory创建描述符
    // 这里暂时返回0
    return 0;
}

uint64_t DX12Texture::GetDefaultSRV() const {
    // 返回SRV句柄
    return reinterpret_cast<uint64_t>(m_srv.ptr);
}

uint64_t DX12Texture::GetDefaultRTV() const {
    // 返回RTV句柄
    return reinterpret_cast<uint64_t>(m_rtv.ptr);
}

uint64_t DX12Texture::GetDefaultDSV() const {
    // 返回DSV句柄
    return reinterpret_cast<uint64_t>(m_dsv.ptr);
}

uint64_t DX12Texture::GetDefaultUAV() const {
    // 返回UAV句柄
    return reinterpret_cast<uint64_t>(m_uav.ptr);
}

void DX12Texture::Clear(const Color& color, uint32_t mipLevel, uint32_t arraySlice) {
    if (!m_resource) {
        return;
    }

    // 需要通过命令缓冲区清除
    // 这里暂时不实现
}

void DX12Texture::ClearDepthStencil(float depth, uint8_t stencil) {
    if (!m_resource || !IsDepthStencil()) {
        return;
    }

    // 需要通过命令缓冲区清除
    // 这里暂时不实现
}

void DX12Texture::ResolveMultisampled(ITexture* dstTexture, TextureFormat format) {
    if (!m_resource || !dstTexture) {
        return;
    }

    // 需要通过命令缓冲区解决多重采样
    // 这里暂时不实现
}

void DX12Texture::Discard(uint32_t mipLevel, uint32_t arraySlice) {
    // 丢弃资源内容
    // 在DirectX12中，这通常通过丢弃映射来实现
    // 这里暂时不实现
}

void DX12Texture::Compact() {
    // 压缩资源
    // 这里暂时不实现
}

uint64_t DX12Texture::GetMemoryUsage() const {
    return GetSize();
}

bool DX12Texture::DebugSaveToFile(const std::string& filename,
                                   uint32_t mipLevel, uint32_t arraySlice) {
    // 将纹理保存到文件
    // 这里需要使用stb_image_write
    // 暂时返回false
    return false;
}

bool DX12Texture::Validate() {
    return m_resource != nullptr;
}

// DirectX12特定方法
void DX12Texture::CreateRTV(D3D12_CPU_DESCRIPTOR_HANDLE handle) {
    m_rtv = handle;

    if (!m_device || !m_resource) {
        return;
    }

    auto device = m_device->GetD3D12Device();
    if (!device) {
        return;
    }

    // 创建渲染目标视图
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = GetDXGIFormat();
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;
    rtvDesc.Texture2D.PlaneSlice = 0;

    device->CreateRenderTargetView(m_resource.Get(), &rtvDesc, handle);
}

void DX12Texture::CreateDSV(D3D12_CPU_DESCRIPTOR_HANDLE handle) {
    m_dsv = handle;

    if (!m_device || !m_resource) {
        return;
    }

    auto device = m_device->GetD3D12Device();
    if (!device) {
        return;
    }

    // 创建深度模板视图
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = GetDXGIFormat();
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;

    device->CreateDepthStencilView(m_resource.Get(), &dsvDesc, handle);
}

void DX12Texture::CreateSRV(D3D12_CPU_DESCRIPTOR_HANDLE handle) {
    if (!m_device || !m_resource) {
        return;
    }

    auto device = m_device->GetD3D12Device();
    if (!device) {
        return;
    }

    // 创建着色器资源视图
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = GetDXGIFormat();
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = m_desc.mipLevels;
    srvDesc.Texture2D.PlaneSlice = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    srvDesc.Texture2D.ResourceMaxLODClamp = FLT_MAX;

    device->CreateShaderResourceView(m_resource.Get(), &srvDesc, handle);
}

void DX12Texture::CreateUAV(D3D12_CPU_DESCRIPTOR_HANDLE handle) {
    if (!m_device || !m_resource) {
        return;
    }

    auto device = m_device->GetD3D12Device();
    if (!device) {
        return;
    }

    // 创建无序访问视图
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = GetDXGIFormat();
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice = 0;
    uavDesc.Texture2D.PlaneSlice = 0;

    device->CreateUnorderedAccessView(m_resource.Get(), &uavDesc, handle);
}

// 辅助方法
D3D12_RESOURCE_DESC DX12Texture::GetD3D12ResourceDesc() const {
    if (m_resource) {
        return m_resource->GetDesc();
    }

    // 创建默认描述
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment = 0;
    desc.Width = m_desc.width;
    desc.Height = m_desc.height;
    desc.DepthOrArraySize = m_desc.arraySize;
    desc.MipLevels = m_desc.mipLevels;
    desc.Format = GetDXGIFormat();
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    if (m_desc.allowRenderTarget) {
        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }
    if (m_desc.allowUnorderedAccess) {
        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    return desc;
}

DXGI_FORMAT DX12Texture::GetDXGIFormat() const {
    // 转换纹理格式
    switch (m_desc.format) {
        case TextureFormat::RGBA8_UNorm: return DXGI_FORMAT_R8G8B8A8_UNORM;
        case TextureFormat::RGBA8_SNorm: return DXGI_FORMAT_R8G8B8A8_SNORM;
        case TextureFormat::RGBA32_Float: return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case TextureFormat::D32_Float: return DXGI_FORMAT_D32_FLOAT;
        case TextureFormat::D24_UNorm_S8_UInt: return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case TextureFormat::D32_Float_S8_UInt: return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
        default: return DXGI_FORMAT_UNKNOWN;
    }
}

D3D12_RESOURCE_STATES DX12Texture::GetInitialState() const {
    if (IsRenderTarget()) {
        return D3D12_RESOURCE_STATE_RENDER_TARGET;
    }
    if (IsDepthStencil()) {
        return D3D12_RESOURCE_STATE_DEPTH_WRITE;
    }
    if (IsShaderResource()) {
        return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }
    return D3D12_RESOURCE_STATE_COMMON;
}

} // namespace PrismaEngine::Graphic::DX12