#include "DX12Buffer.h"
#include "DX12RenderDevice.h"
#include <directx/d3dx12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace PrismaEngine::Graphic::DX12 {

DX12Buffer::DX12Buffer(DX12RenderDevice* device,
                       ComPtr<ID3D12Resource> resource,
                       const BufferDesc& desc)
    : m_device(device)
    , m_resource(resource)
    , m_desc(desc) {

    // 计算步长
    if (desc.stride == 0) {
        switch (desc.type) {
            case BufferType::Vertex:
                m_stride = 32; // 默认顶点大小
                break;
            case BufferType::Index:
                m_stride = sizeof(uint32_t); // 索引使用32位
                break;
            case BufferType::Constant:
                m_stride = 256; // 常量缓冲区256字节对齐
                break;
            default:
                m_stride = 1;
                break;
        }
        m_desc.stride = m_stride;
    }

    // 设置资源ID
    m_id = static_cast<ResourceId>(reinterpret_cast<uintptr_t>(resource.Get()));

    // 获取CPU地址（如果是上传缓冲区）
    if (IsUploadHeap()) {
        D3D12_RANGE readRange = {0, 0};
        resource->Map(0, &readRange, &m_cpuAddress);
    }
}

DX12Buffer::~DX12Buffer() {
    // 如果映射了，取消映射
    if (m_cpuAddress && IsUploadHeap()) {
        m_resource->Unmap(0, nullptr);
    }
}

// IBuffer接口实现
BufferType DX12Buffer::GetBufferType() const {
    return m_desc.type;
}

uint64_t DX12Buffer::GetSize() const {
    return m_desc.size;
}

uint32_t DX12Buffer::GetStride() const {
    return m_stride;
}

BufferUsage DX12Buffer::GetUsage() const {
    return m_desc.usage;
}

uint32_t DX12Buffer::GetElementCount() const {
    if (m_stride > 0) {
        return static_cast<uint32_t>(m_desc.size / m_stride);
    }
    return 0;
}

bool DX12Buffer::IsDynamic() const {
    return HasFlag(m_desc.usage, BufferUsage::Dynamic);
}

bool DX12Buffer::IsReadOnly() const {
    return !HasFlag(m_desc.usage, BufferUsage::UnorderedAccess);
}

bool DX12Buffer::IsShaderResource() const {
    return HasFlag(m_desc.usage, BufferUsage::ShaderResource);
}

bool DX12Buffer::IsUnorderedAccess() const {
    return HasFlag(m_desc.usage, BufferUsage::UnorderedAccess);
}

BufferMapDesc DX12Buffer::Map(uint64_t offset, uint64_t size, uint32_t mapType) {
    if (!m_resource || m_mapped || !IsUploadHeap()) {
        return {};
    }

    D3D12_RANGE range = {offset, size > 0 ? size : m_desc.size - offset};
    void* data = nullptr;

    HRESULT hr = m_resource->Map(0, &range, &data);
    if (FAILED(hr)) {
        return {};
    }

    m_mapped = true;

    BufferMapDesc mapDesc;
    mapDesc.data = static_cast<uint8_t*>(data) + offset;
    mapDesc.size = size;
    mapDesc.offset = offset;

    return mapDesc;
}

void DX12Buffer::Unmap(uint64_t offset, uint64_t size) {
    if (!m_resource || !m_mapped) {
        return;
    }

    D3D12_RANGE range = {offset, size > 0 ? size : m_desc.size - offset};
    m_resource->Unmap(0, &range);

    m_mapped = false;
}

void DX12Buffer::UpdateData(const void* data, uint64_t size, uint64_t offset) {
    if (!m_device || !data || offset + size > m_desc.size) {
        return;
    }

    // 对于上传缓冲区，直接映射并复制
    if (IsUploadHeap()) {
        auto mapDesc = Map(offset, size, 0);
        if (mapDesc.data) {
            memcpy(mapDesc.data, data, size);
            Unmap(offset, size);
        }
        return;
    }

    // 对于其他类型的缓冲区，需要通过命令缓冲区更新
    auto cmdBuffer = m_device->CreateCommandBuffer(CommandBufferType::Graphics);
    if (cmdBuffer) {
        cmdBuffer->Begin();
        cmdBuffer->UpdateBuffer(this, data, size, offset);
        cmdBuffer->End();
        cmdBuffer->Close();

        m_device->SubmitCommandBuffer(cmdBuffer.get());
        m_device->WaitForIdle();
    }
}

bool DX12Buffer::ReadData(void* dstBuffer, uint64_t size, uint64_t offset) {
    if (!m_resource || !dstBuffer || offset + size > m_desc.size) {
        return false;
    }

    // 只有读回缓冲区可以映射
    if (!IsReadbackHeap()) {
        return false;
    }

    auto mapDesc = Map(offset, size, 1);
    if (!mapDesc.data) {
        return false;
    }

    memcpy(dstBuffer, mapDesc.data, size);
    Unmap(offset, size);
    return true;
}

void DX12Buffer::CopyTo(IBuffer* dstBuffer,
                      uint64_t srcOffset, uint64_t dstOffset,
                      uint64_t size) {
    auto dstDX12 = static_cast<DX12Buffer*>(dstBuffer);
    if (!dstDX12 || !m_resource) {
        return;
    }

    // 需要通过命令缓冲区执行复制
    auto cmdBuffer = m_device->CreateCommandBuffer(CommandBufferType::Graphics);
    if (cmdBuffer) {
        cmdBuffer->Begin();
        cmdBuffer->CopyBufferRegion(dstBuffer, dstOffset, this, srcOffset, size);
        cmdBuffer->End();
        cmdBuffer->Close();

        m_device->SubmitCommandBuffer(cmdBuffer.get());
        m_device->WaitForIdle();
    }
}

void DX12Buffer::Fill(uint32_t value, uint64_t offset, uint64_t size) {
    uint32_t* data = new uint32_t[size / 4];
    std::fill_n(data, size / 4, value);

    UpdateData(data, size, offset);
    delete[] data;
}

void DX12Buffer::CopyFromTexture(ITexture* srcTexture,
                              uint32_t srcMipLevel, uint32_t srcArraySlice) {
    // 从纹理复制数据到缓冲区
    // 需要通过命令缓冲区执行
    // 这里暂时不实现
}

void DX12Buffer::CopyToTexture(ITexture* dstTexture,
                              uint32_t dstMipLevel, uint32_t dstArraySlice) {
    auto dstDX12 = static_cast<ITexture*>(dstTexture);
    if (!dstDX12 || !m_resource) {
        return;
    }

    // 从缓冲区复制数据到纹理
    // 需要通过命令缓冲区执行
    // 这里暂时不实现
}

uint64_t DX12Buffer::CreateView(BufferDescriptorType descType, const BufferViewDesc& desc) {
    // 创建缓冲区视图
    // 需要通过DX12ResourceFactory创建
    // 这里暂时返回0
    return 0;
}

uint64_t DX12Buffer::GetDefaultSRV() const {
    return m_srv.ptr;
}

uint64_t DX12Buffer::GetDefaultUAV() const {
    return m_uav.ptr;
}

uint64_t DX12Buffer::GetDefaultCBV() const {
    return m_cbv.ptr;
}

uint64_t DX12Buffer::GetDefaultVBV() const {
    return m_vbv.ptr;
}

uint64_t DX12Buffer::GetDefaultIBV() const {
    return m_ibv.ptr;
}

uint64_t DX12Buffer::AllocateDynamic(uint64_t size, uint64_t alignment) {
    // 对于动态缓冲区，这里只是返回偏移量
    // 实际的动态分配需要在DX12ResourceFactory中实现
    if (m_dynamicOffset + size > m_dynamicSize) {
        return 0; // 空间不足
    }

    uint64_t offset = m_dynamicOffset;
    m_dynamicOffset = (offset + size + alignment - 1) & ~(alignment - 1);
    return offset;
}

void DX12Buffer::ResetDynamicAllocation() {
    m_dynamicOffset = 0;
}

uint64_t DX12Buffer::GetCurrentDynamicOffset() const {
    return m_dynamicOffset;
}

uint64_t DX12Buffer::GetAvailableDynamicSpace() const {
    return m_dynamicSize - m_dynamicOffset;
}

bool DX12Buffer::DebugSaveToFile(const std::string& filename,
                                 const std::string& format,
                                 uint64_t offset, uint64_t size) {
    // 保存缓冲区内容到文件
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        return false;
    }

    // 映射缓冲区
    auto mapDesc = Map(offset, size, 1);
    if (!mapDesc.data) {
        return false;
    }

    file.write(reinterpret_cast<char*>(mapDesc.data), size);
    Unmap(offset, size);

    return true;
}

bool DX12Buffer::DebugValidateContent(const void* expectedData,
                                    uint64_t size, uint64_t offset) {
    if (!expectedData) {
        return false;
    }

    // 映射缓冲区进行比较
    auto mapDesc = Map(offset, size, 1);
    if (!mapDesc.data) {
        return false;
    }

    bool result = (memcmp(mapDesc.data, expectedData, size) == 0);
    Unmap(offset, size);

    return result;
}

void DX12Buffer::DebugPrintInfo() const {
    std::stringstream ss;
    ss << "Buffer Info:\n";
    ss << "  Type: " << static_cast<int>(m_desc.type) << "\n";
    ss << "  Size: " << m_desc.size << " bytes\n";
    ss << "  Stride: " << m_stride << " bytes\n";
    ss << "  ElementCount: " << GetElementCount() << "\n";
    ss << "  Usage: " << static_cast<int>(m_desc.usage) << "\n";

    // 输出日志
    Logger::Info("Buffer", ss.str());
}

void DX12Buffer::Discard(uint64_t offset, uint64_t size) {
    // 丢弃缓冲区内容
    // 在DirectX12中，这通常通过映射指定区域来实现
    // 这里暂时不实现
}

void DX12Buffer::Reserve(uint64_t size) {
    // 预分配空间
    // 这里暂时不实现，需要在创建时就指定大小
}

void DX12Buffer::Compact() {
    // 压缩缓冲区
    // 这里暂时不实现
}

uint64_t DX12Buffer::GetMemoryUsage() const {
    return GetSize();
}

uint64_t DX12Buffer::GetGPUMemoryUsage() const {
    return GetSize();
}

// DirectX12特定方法
D3D12_RESOURCE_DESC DX12Buffer::GetD3D12ResourceDesc() const {
    if (m_resource) {
        return m_resource->GetDesc();
    }

    // 创建默认描述
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = 0;
    desc.Width = m_desc.size;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    if (HasFlag(m_desc.usage, BufferUsage::ShaderResource)) {
        // 注意：D3D12_RESOURCE_FLAG_ALLOW_SHADER_RESOURCE 不是有效标志
        // 缓冲区资源默认就是着色器资源，除非明确拒绝
    }
    if (HasFlag(m_desc.usage, BufferUsage::UnorderedAccess)) {
        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    return desc;
}

D3D12_HEAP_TYPE DX12Buffer::GetHeapType() const {
    // 根据缓冲区用途确定堆类型
    if (IsDynamic()) {
        return D3D12_HEAP_TYPE_UPLOAD;
    }
    if (IsReadOnly()) {
        return D3D12_HEAP_TYPE_READBACK;
    }
    return D3D12_HEAP_TYPE_DEFAULT;
}

D3D12_HEAP_FLAGS DX12Buffer::GetHeapFlags() const {
    // 缓冲区通常不需要特殊的堆标志
    return D3D12_HEAP_FLAG_NONE;
}

bool DX12Buffer::IsUploadHeap() const {
    return GetHeapType() == D3D12_HEAP_TYPE_UPLOAD;
}

bool DX12Buffer::IsReadbackHeap() const {
    return GetHeapType() == D3D12_HEAP_TYPE_READBACK;
}

bool DX12Buffer::IsDefaultHeap() const {
    return GetHeapType() == D3D12_HEAP_TYPE_DEFAULT;
}

// 辅助方法
D3D12_RESOURCE_STATES DX12Buffer::GetInitialResourceState() const {
    if (HasFlag(m_desc.usage, BufferUsage::ShaderResource)) {
        return D3D12_RESOURCE_STATE_COPY_DEST;
    }
    if (HasFlag(m_desc.usage, BufferUsage::UnorderedAccess)) {
        return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }
    return D3D12_RESOURCE_STATE_GENERIC_READ;
}

} // namespace PrismaEngine::Graphic::DX12