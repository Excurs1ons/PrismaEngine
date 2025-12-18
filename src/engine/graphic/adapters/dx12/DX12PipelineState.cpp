#include "DX12PipelineState.h"
#include "DX12RenderDevice.h"
#include "../Logger.h"
#include <directx/d3dx12.h>
#include <wrl/client.h>
#include <sstream>
#include <algorithm>

using Microsoft::WRL::ComPtr;

namespace PrismaEngine::Graphic::DX12 {

DX12PipelineState::DX12PipelineState(DX12RenderDevice* device)
    : m_device(device)
    , m_blendStates(8)  // 默认8个渲染目标
    , m_cacheKey(0) {

    // 设置默认状态
    m_topology = PrimitiveTopology::TriangleList;
    m_rasterizerState = RasterizerState::Default;
    m_depthStencilState = DepthStencilState::Default;
    m_sampleCount = 1;
    m_sampleQuality = 0;
    m_depthStencilFormat = TextureFormat::D32_Float;
}

DX12PipelineState::~DX12PipelineState() {
    // 资源由ComPtr自动管理
}

// IPipelineState接口实现
PipelineType DX12PipelineState::GetType() const {
    return m_type;
}

bool DX12PipelineState::IsValid() const {
    return m_pipelineState != nullptr && m_rootSignature != nullptr;
}

// 着色器管理
void DX12PipelineState::SetShader(ShaderType type, std::shared_ptr<IShader> shader) {
    uint32_t index = static_cast<uint32_t>(type);
    if (index < m_shaders.size()) {
        m_shaders[index] = shader;

        // 如果是第一个非计算着色器，确定管线类型
        if (m_type == PipelineType::Graphics && type == ShaderType::Compute) {
            m_type = PipelineType::Compute;
        } else if (m_type == PipelineType::Compute && type != ShaderType::Compute) {
            m_type = PipelineType::Graphics;
        }
    }
}

std::shared_ptr<IShader> DX12PipelineState::GetShader(ShaderType type) const {
    uint32_t index = static_cast<uint32_t>(type);
    if (index < m_shaders.size()) {
        return m_shaders[index];
    }
    return nullptr;
}

bool DX12PipelineState::HasShader(ShaderType type) const {
    return GetShader(type) != nullptr;
}

// 渲染状态
void DX12PipelineState::SetPrimitiveTopology(PrimitiveTopology topology) {
    m_topology = topology;
}

PrimitiveTopology DX12PipelineState::GetPrimitiveTopology() const {
    return m_topology;
}

void DX12PipelineState::SetBlendState(const BlendState& state, uint32_t renderTargetIndex) {
    if (renderTargetIndex >= m_blendStates.size()) {
        m_blendStates.resize(renderTargetIndex + 1);
    }
    m_blendStates[renderTargetIndex] = state;
}

const BlendState& DX12PipelineState::GetBlendState(uint32_t renderTargetIndex) const {
    if (renderTargetIndex < m_blendStates.size()) {
        return m_blendStates[renderTargetIndex];
    }
    static BlendState defaultState = BlendState::Default;
    return defaultState;
}

void DX12PipelineState::SetRasterizerState(const RasterizerState& state) {
    m_rasterizerState = state;
}

const RasterizerState& DX12PipelineState::GetRasterizerState() const {
    return m_rasterizerState;
}

void DX12PipelineState::SetDepthStencilState(const DepthStencilState& state) {
    m_depthStencilState = state;
}

const DepthStencilState& DX12PipelineState::GetDepthStencilState() const {
    return m_depthStencilState;
}

// 顶点输入布局
void DX12PipelineState::SetInputLayout(const std::vector<VertexInputAttribute>& attributes) {
    m_inputLayout = attributes;
}

const std::vector<VertexInputAttribute>& DX12PipelineState::GetInputLayout() const {
    return m_inputLayout;
}

uint32_t DX12PipelineState::GetInputAttributeCount() const {
    return static_cast<uint32_t>(m_inputLayout.size());
}

// 渲染目标格式
void DX12PipelineState::SetRenderTargetFormats(const std::vector<TextureFormat>& formats) {
    m_renderTargetFormats = formats;
}

void DX12PipelineState::SetRenderTargetFormat(uint32_t index, TextureFormat format) {
    if (index >= m_renderTargetFormats.size()) {
        m_renderTargetFormats.resize(index + 1);
    }
    m_renderTargetFormats[index] = format;
}

TextureFormat DX12PipelineState::GetRenderTargetFormat(uint32_t index) const {
    if (index < m_renderTargetFormats.size()) {
        return m_renderTargetFormats[index];
    }
    return TextureFormat::Unknown;
}

uint32_t DX12PipelineState::GetRenderTargetCount() const {
    return static_cast<uint32_t>(m_renderTargetFormats.size());
}

void DX12PipelineState::SetDepthStencilFormat(TextureFormat format) {
    m_depthStencilFormat = format;
}

TextureFormat DX12PipelineState::GetDepthStencilFormat() const {
    return m_depthStencilFormat;
}

// 多重采样
void DX12PipelineState::SetSampleCount(uint32_t sampleCount, uint32_t sampleQuality) {
    m_sampleCount = sampleCount;
    m_sampleQuality = sampleQuality;
}

uint32_t DX12PipelineState::GetSampleCount() const {
    return m_sampleCount;
}

uint32_t DX12PipelineState::GetSampleQuality() const {
    return m_sampleQuality;
}

// 创建和验证
bool DX12PipelineState::Create(IRenderDevice* device) {
    m_errors.clear();

    // 创建根签名
    if (!CreateD3D12RootSignature()) {
        m_errors = "Failed to create root signature";
        return false;
    }

    // 获取DX12设备
    if (!m_device) {
        m_errors = "DX12 device not available";
        return false;
    }

    auto d3d12Device = m_device->GetD3D12Device();
    if (!d3d12Device) {
        m_errors = "D3D12 device not available";
        return false;
    }

    // 根据管线类型创建不同的PSO
    HRESULT hr = E_FAIL;
    if (m_type == PipelineType::Graphics) {
        auto desc = CreateD3D12PipelineDesc();
        hr = d3d12Device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&m_pipelineState));
    } else {
        auto desc = CreateD3D12ComputePipelineDesc();
        hr = d3d12Device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&m_pipelineState));
    }

    if (FAILED(hr)) {
        std::stringstream ss;
        ss << "Failed to create pipeline state: HRESULT 0x" << std::hex << hr;
        m_errors = ss.str();
        return false;
    }

    // 计算缓存键
    m_cacheKey = CalculateCacheKey();

    return true;
}

bool DX12PipelineState::Recreate() {
    // 清理旧资源
    m_pipelineState.Reset();
    m_rootSignature.Reset();

    // 重新创建
    return Create(nullptr);
}

bool DX12PipelineState::Validate(IRenderDevice* device, std::string& errors) const {
    errors.clear();

    if (m_type == PipelineType::Graphics) {
        return ValidateGraphicsPipeline(errors);
    } else {
        return ValidateComputePipeline(errors);
    }
}

// 缓存
uint64_t DX12PipelineState::GetCacheKey() const {
    return m_cacheKey;
}

bool DX12PipelineState::LoadFromCache(IRenderDevice* device, uint64_t cacheKey) {
    // TODO: 实现从缓存加载
    m_cacheKey = cacheKey;
    return false;
}

bool DX12PipelineState::SaveToCache() const {
    // TODO: 实现保存到缓存
    return false;
}

// 调试
const std::string& DX12PipelineState::GetErrors() const {
    return m_errors;
}

void DX12PipelineState::SetDebugName(const std::string& name) {
    m_debugName = name;
    if (m_pipelineState) {
        m_pipelineState->SetName(std::wstring(name.begin(), name.end()).c_str());
    }
    if (m_rootSignature) {
        m_rootSignature->SetName(std::wstring(name.begin(), name.end()).c_str());
    }
}

const std::string& DX12PipelineState::GetDebugName() const {
    return m_debugName;
}

// 克隆
std::unique_ptr<IPipelineState> DX12PipelineState::Clone() const {
    auto clone = std::make_unique<DX12PipelineState>(m_device);

    // 复制状态
    clone->m_type = m_type;
    clone->m_topology = m_topology;
    clone->m_rasterizerState = m_rasterizerState;
    clone->m_depthStencilState = m_depthStencilState;
    clone->m_inputLayout = m_inputLayout;
    clone->m_renderTargetFormats = m_renderTargetFormats;
    clone->m_depthStencilFormat = m_depthStencilFormat;
    clone->m_sampleCount = m_sampleCount;
    clone->m_sampleQuality = m_sampleQuality;
    clone->m_blendStates = m_blendStates;
    clone->m_debugName = m_debugName;

    // 复制着色器
    clone->m_shaders = m_shaders;

    return std::move(clone);
}

// DirectX12特定方法实现

D3D12_GRAPHICS_PIPELINE_STATE_DESC DX12PipelineState::CreateD3D12PipelineDesc() const {
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};
    desc.pRootSignature = m_rootSignature.Get();

    // 设置着色器
    auto vertexShader = GetShader(ShaderType::Vertex);
    auto pixelShader = GetShader(ShaderType::Pixel);
    auto geometryShader = GetShader(ShaderType::Geometry);
    auto hullShader = GetShader(ShaderType::Hull);
    auto domainShader = GetShader(ShaderType::Domain);

    if (vertexShader) {
        auto dx12VS = static_cast<DX12Shader*>(vertexShader.get());
        desc.VS = CD3DX12_SHADER_BYTECODE(dx12VS->GetBytecodeData(), dx12VS->GetBytecodeSize());
    }
    if (pixelShader) {
        auto dx12PS = static_cast<DX12Shader*>(pixelShader.get());
        desc.PS = CD3DX12_SHADER_BYTECODE(dx12PS->GetBytecodeData(), dx12PS->GetBytecodeSize());
    }
    if (geometryShader) {
        auto dx12GS = static_cast<DX12Shader*>(geometryShader.get());
        desc.GS = CD3DX12_SHADER_BYTECODE(dx12GS->GetBytecodeData(), dx12GS->GetBytecodeSize());
    }
    if (hullShader) {
        auto dx12HS = static_cast<DX12Shader*>(hullShader.get());
        desc.HS = CD3DX12_SHADER_BYTECODE(dx12HS->GetBytecodeData(), dx12HS->GetBytecodeSize());
    }
    if (domainShader) {
        auto dx12DS = static_cast<DX12Shader*>(domainShader.get());
        desc.DS = CD3DX12_SHADER_BYTECODE(dx12DS->GetBytecodeData(), dx12DS->GetBytecodeSize());
    }

    // 设置流输出（流化输出，如果需要）
    desc.StreamOutput = {};

    // 设置混合状态
    desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    if (m_blendStates.empty()) {
        // 使用默认混合状态
        desc.BlendState.RenderTarget[0] = {};
        desc.BlendState.RenderTarget[0].BlendEnable = FALSE;
        desc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
        desc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
        desc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
        desc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        desc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        desc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        desc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        desc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
        desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    } else {
        for (size_t i = 0; i < m_blendStates.size() && i < 8; ++i) {
            const auto& blend = m_blendStates[i];
            desc.BlendState.RenderTarget[i].BlendEnable = blend.blendEnable;
            desc.BlendState.RenderTarget[i].LogicOpEnable = blend.logicOpEnable;
            desc.BlendState.RenderTarget[i].SrcBlend = GetD3D12Blend(blend.srcBlend);
            desc.BlendState.RenderTarget[i].DestBlend = GetD3D12Blend(blend.destBlend);
            desc.BlendState.RenderTarget[i].BlendOp = GetD3D12BlendOp(blend.blendOp);
            desc.BlendState.RenderTarget[i].SrcBlendAlpha = GetD3D12Blend(blend.srcBlendAlpha);
            desc.BlendState.RenderTarget[i].DestBlendAlpha = GetD3D12Blend(blend.destBlendAlpha);
            desc.BlendState.RenderTarget[i].BlendOpAlpha = GetD3D12BlendOp(blend.blendOpAlpha);
            desc.BlendState.RenderTarget[i].RenderTargetWriteMask = blend.writeMask;
        }
    }

    // 设置采样掩码
    desc.SampleMask = UINT_MAX;

    // 设置光栅化状态
    desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    desc.RasterizerState.FillMode = GetD3D12FillMode();
    desc.RasterizerState.CullMode = GetD3D12CullMode();
    desc.RasterizerState.FrontCounterClockwise = m_rasterizerState.frontCounterClockwise;
    desc.RasterizerState.DepthBias = m_rasterizerState.depthBias;
    desc.RasterizerState.DepthBiasClamp = m_rasterizerState.depthBiasClamp;
    desc.RasterizerState.SlopeScaledDepthBias = m_rasterizerState.slopeScaledDepthBias;
    desc.RasterizerState.DepthClipEnable = m_rasterizerState.depthClipEnable;
    desc.RasterizerState.MultisampleEnable = m_rasterizerState.multisampleEnable;
    desc.RasterizerState.AntialiasedLineEnable = m_rasterizerState.antialiasedLineEnable;
    desc.RasterizerState.ForcedSampleCount = 0;
    desc.RasterizerState.ConservativeRaster = m_rasterizerState.conservativeRaster ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    // 设置深度模板状态
    desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    desc.DepthStencilState.DepthEnable = m_depthStencilState.depthEnable;
    desc.DepthStencilState.DepthWriteMask = m_depthStencilState.depthWriteMask ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
    desc.DepthStencilState.DepthFunc = GetD3D12ComparisonFunc(m_depthStencilState.depthFunc);
    desc.DepthStencilState.StencilEnable = m_depthStencilState.stencilEnable;
    desc.DepthStencilState.StencilReadMask = m_depthStencilState.stencilReadMask;
    desc.DepthStencilState.StencilWriteMask = m_depthStencilState.stencilWriteMask;
    desc.DepthStencilState.FrontFace.StencilFailOp = static_cast<D3D12_STENCIL_OP>(m_depthStencilState.frontFace.failOp);
    desc.DepthStencilState.FrontFace.StencilDepthFailOp = static_cast<D3D12_STENCIL_OP>(m_depthStencilState.frontFace.depthFailOp);
    desc.DepthStencilState.FrontFace.StencilPassOp = static_cast<D3D12_STENCIL_OP>(m_depthStencilState.frontFace.passOp);
    desc.DepthStencilState.FrontFace.StencilFunc = GetD3D12ComparisonFunc(m_depthStencilState.frontFace.func);
    desc.DepthStencilState.BackFace.StencilFailOp = static_cast<D3D12_STENCIL_OP>(m_depthStencilState.backFace.failOp);
    desc.DepthStencilState.BackFace.StencilDepthFailOp = static_cast<D3D12_STENCIL_OP>(m_depthStencilState.backFace.depthFailOp);
    desc.DepthStencilState.BackFace.StencilPassOp = static_cast<D3D12_STENCIL_OP>(m_depthStencilState.backFace.passOp);
    desc.DepthStencilState.BackFace.StencilFunc = GetD3D12ComparisonFunc(m_depthStencilState.backFace.func);

    // 设置输入布局
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements;
    CreateInputLayout(inputElements);
    desc.InputLayout.pInputElementDescs = inputElements.data();
    desc.InputLayout.NumElements = static_cast<UINT>(inputElements.size());

    // 设置图元拓扑
    desc.PrimitiveTopologyType = GetD3D12PrimitiveTopology();

    // 设置渲染目标格式
    for (size_t i = 0; i < m_renderTargetFormats.size() && i < 8; ++i) {
        desc.RTVFormats[i] = GetDXGIFormat(m_renderTargetFormats[i]);
    }
    desc.NumRenderTargets = static_cast<UINT>(std::min(m_renderTargetFormats.size(), size_t(8)));

    // 设置深度模板格式
    desc.DSVFormat = GetDXGIFormat(m_depthStencilFormat);

    // 设置多重采样
    desc.SampleDesc.Count = m_sampleCount;
    desc.SampleDesc.Quality = m_sampleQuality;

    // 设置节点掩码
    desc.NodeMask = 0;

    // 设置缓存PSO标志
    desc.CachedPSO.pCachedBlob = nullptr;
    desc.CachedPSO.CachedBlobSizeInBytes = 0;

    // 设置标志
    desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

    return desc;
}

D3D12_COMPUTE_PIPELINE_STATE_DESC DX12PipelineState::CreateD3D12ComputePipelineDesc() const {
    D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
    desc.pRootSignature = m_rootSignature.Get();

    // 设置计算着色器
    auto computeShader = GetShader(ShaderType::Compute);
    if (computeShader) {
        auto dx12CS = static_cast<DX12Shader*>(computeShader.get());
        desc.CS = CD3DX12_SHADER_BYTECODE(dx12CS->GetBytecodeData(), dx12CS->GetBytecodeSize());
    }

    // 设置节点掩码
    desc.NodeMask = 0;

    // 设置缓存PSO
    desc.CachedPSO.pCachedBlob = nullptr;
    desc.CachedPSO.CachedBlobSizeInBytes = 0;

    // 设置标志
    desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

    return desc;
}

bool DX12PipelineState::CreateD3D12RootSignature() {
    if (!m_device) {
        return false;
    }

    auto d3d12Device = m_device->GetD3D12Device();
    if (!d3d12Device) {
        return false;
    }

    auto rootSignatureDesc = CreateRootSignatureDesc();

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;

    HRESULT hr = D3DX12SerializeVersionedRootSignature(
        &rootSignatureDesc,
        D3D_ROOT_SIGNATURE_VERSION_1_0,
        &signature,
        &error
    );

    if (FAILED(hr)) {
        if (error) {
            m_errors = std::string(
                static_cast<char*>(error->GetBufferPointer()),
                error->GetBufferSize()
            );
        }
        return false;
    }

    hr = d3d12Device->CreateRootSignature(
        0,
        signature->GetBufferPointer(),
        signature->GetBufferSize(),
        IID_PPV_ARGS(&m_rootSignature)
    );

    return SUCCEEDED(hr);
}

// 私有辅助方法实现

D3D12_PRIMITIVE_TOPOLOGY_TYPE DX12PipelineState::GetD3D12PrimitiveTopology() const {
    switch (m_primitiveTopology) {
        case PrimitiveTopology::PointList: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
        case PrimitiveTopology::LineList: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        case PrimitiveTopology::LineStrip: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        case PrimitiveTopology::TriangleList: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        case PrimitiveTopology::TriangleStrip: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        case PrimitiveTopology::PatchList_1ControlPoints:
        case PrimitiveTopology::PatchList_2ControlPoints:
        case PrimitiveTopology::PatchList_3ControlPoints:
        case PrimitiveTopology::PatchList_4ControlPoints:
        case PrimitiveTopology::PatchList_5ControlPoints:
        case PrimitiveTopology::PatchList_6ControlPoints:
        case PrimitiveTopology::PatchList_7ControlPoints:
        case PrimitiveTopology::PatchList_8ControlPoints:
        case PrimitiveTopology::PatchList_9ControlPoints:
        case PrimitiveTopology::PatchList_10ControlPoints:
        case PrimitiveTopology::PatchList_11ControlPoints:
        case PrimitiveTopology::PatchList_12ControlPoints:
        case PrimitiveTopology::PatchList_13ControlPoints:
        case PrimitiveTopology::PatchList_14ControlPoints:
        case PrimitiveTopology::PatchList_15ControlPoints:
        case PrimitiveTopology::PatchList_16ControlPoints:
        case PrimitiveTopology::PatchList_17ControlPoints:
        case PrimitiveTopology::PatchList_18ControlPoints:
        case PrimitiveTopology::PatchList_19ControlPoints:
        case PrimitiveTopology::PatchList_20ControlPoints:
        case PrimitiveTopology::PatchList_21ControlPoints:
        case PrimitiveTopology::PatchList_22ControlPoints:
        case PrimitiveTopology::PatchList_23ControlPoints:
        case PrimitiveTopology::PatchList_24ControlPoints:
        case PrimitiveTopology::PatchList_25ControlPoints:
        case PrimitiveTopology::PatchList_26ControlPoints:
        case PrimitiveTopology::PatchList_27ControlPoints:
        case PrimitiveTopology::PatchList_28ControlPoints:
        case PrimitiveTopology::PatchList_29ControlPoints:
        case PrimitiveTopology::PatchList_30ControlPoints:
        case PrimitiveTopology::PatchList_31ControlPoints:
        case PrimitiveTopology::PatchList_32ControlPoints:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
        default: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
    }
}

D3D12_FILL_MODE DX12PipelineState::GetD3D12FillMode() const {
    switch (m_rasterizerState.fillMode) {
        case FillMode::Wireframe: return D3D12_FILL_MODE_WIREFRAME;
        case FillMode::Solid: return D3D12_FILL_MODE_SOLID;
        default: return D3D12_FILL_MODE_SOLID;
    }
}

D3D12_CULL_MODE DX12PipelineState::GetD3D12CullMode() const {
    switch (m_rasterizerState.cullMode) {
        case CullMode::None: return D3D12_CULL_MODE_NONE;
        case CullMode::Front: return D3D12_CULL_MODE_FRONT;
        case CullMode::Back: return D3D12_CULL_MODE_BACK;
        default: return D3D12_CULL_MODE_BACK;
    }
}

D3D12_COMPARISON_FUNC DX12PipelineState::GetD3D12ComparisonFunc(ComparisonFunc func) const {
    switch (func) {
        case ComparisonFunc::Never: return D3D12_COMPARISON_FUNC_NEVER;
        case ComparisonFunc::Less: return D3D12_COMPARISON_FUNC_LESS;
        case ComparisonFunc::Equal: return D3D12_COMPARISON_FUNC_EQUAL;
        case ComparisonFunc::LessEqual: return D3D12_COMPARISON_FUNC_LESS_EQUAL;
        case ComparisonFunc::Greater: return D3D12_COMPARISON_FUNC_GREATER;
        case ComparisonFunc::NotEqual: return D3D12_COMPARISON_FUNC_NOT_EQUAL;
        case ComparisonFunc::GreaterEqual: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        case ComparisonFunc::Always: return D3D12_COMPARISON_FUNC_ALWAYS;
        default: return D3D12_COMPARISON_FUNC_LESS;
    }
}

D3D12_BLEND_OP DX12PipelineState::GetD3D12BlendOp(BlendOp op) const {
    switch (op) {
        case BlendOp::Add: return D3D12_BLEND_OP_ADD;
        case BlendOp::Subtract: return D3D12_BLEND_OP_SUBTRACT;
        case BlendOp::RevSubtract: return D3D12_BLEND_OP_REV_SUBTRACT;
        case BlendOp::Min: return D3D12_BLEND_OP_MIN;
        case BlendOp::Max: return D3D12_BLEND_OP_MAX;
        default: return D3D12_BLEND_OP_ADD;
    }
}

D3D12_BLEND DX12PipelineState::GetD3D12Blend(BlendFactor factor) const {
    switch (factor) {
        case BlendFactor::Zero: return D3D12_BLEND_ZERO;
        case BlendFactor::One: return D3D12_BLEND_ONE;
        case BlendFactor::SrcColor: return D3D12_BLEND_SRC_COLOR;
        case BlendFactor::InvSrcColor: return D3D12_BLEND_INV_SRC_COLOR;
        case BlendFactor::SrcAlpha: return D3D12_BLEND_SRC_ALPHA;
        case BlendFactor::InvSrcAlpha: return D3D12_BLEND_INV_SRC_ALPHA;
        case BlendFactor::DstAlpha: return D3D12_BLEND_DEST_ALPHA;
        case BlendFactor::InvDstAlpha: return D3D12_BLEND_INV_DEST_ALPHA;
        case BlendFactor::DstColor: return D3D12_BLEND_DEST_COLOR;
        case BlendFactor::InvDstColor: return D3D12_BLEND_INV_DEST_COLOR;
        case BlendFactor::SrcAlphaSat: return D3D12_BLEND_SRC_ALPHA_SAT;
        case BlendFactor::BlendFactor: return D3D12_BLEND_BLEND_FACTOR;
        case BlendFactor::InvBlendFactor: return D3D12_BLEND_INV_BLEND_FACTOR;
        case BlendFactor::Src1Color: return D3D12_BLEND_SRC1_COLOR;
        case BlendFactor::InvSrc1Color: return D3D12_BLEND_INV_SRC1_COLOR;
        case BlendFactor::Src1Alpha: return D3D12_BLEND_SRC1_ALPHA;
        case BlendFactor::InvSrc1Alpha: return D3D12_BLEND_INV_SRC1_ALPHA;
        default: return D3D12_BLEND_ONE;
    }
}

DXGI_FORMAT DX12PipelineState::GetDXGIFormat(TextureFormat format) const {
    switch (format) {
        case TextureFormat::Unknown: return DXGI_FORMAT_UNKNOWN;
        case TextureFormat::R32_Float: return DXGI_FORMAT_R32_FLOAT;
        case TextureFormat::R32_UInt: return DXGI_FORMAT_R32_UINT;
        case TextureFormat::R32_SInt: return DXGI_FORMAT_R32_SINT;
        case TextureFormat::R16_Float: return DXGI_FORMAT_R16_FLOAT;
        case TextureFormat::R16_UInt: return DXGI_FORMAT_R16_UINT;
        case TextureFormat::R16_SInt: return DXGI_FORMAT_R16_SINT;
        case TextureFormat::R8_UNorm: return DXGI_FORMAT_R8_UNORM;
        case TextureFormat::R8_SNorm: return DXGI_FORMAT_R8_SNORM;
        case TextureFormat::RG32_Float: return DXGI_FORMAT_R32G32_FLOAT;
        case TextureFormat::RG32_UInt: return DXGI_FORMAT_R32G32_UINT;
        case TextureFormat::RG32_SInt: return DXGI_FORMAT_R32G32_SINT;
        case TextureFormat::RG16_Float: return DXGI_FORMAT_R16G16_FLOAT;
        case TextureFormat::RG16_UInt: return DXGI_FORMAT_R16G16_UINT;
        case TextureFormat::RG16_SInt: return DXGI_FORMAT_R16G16_SINT;
        case TextureFormat::RG8_UNorm: return DXGI_FORMAT_R8G8_UNORM;
        case TextureFormat::RG8_SNorm: return DXGI_FORMAT_R8G8_SNORM;
        case TextureFormat::RGB32_Float: return DXGI_FORMAT_R32G32B32_FLOAT;
        case TextureFormat::RGB32_UInt: return DXGI_FORMAT_R32G32B32_UINT;
        case TextureFormat::RGB32_SInt: return DXGI_FORMAT_R32G32B32_SINT;
        case TextureFormat::RGBA8_UNorm: return DXGI_FORMAT_R8G8B8A8_UNORM;
        case TextureFormat::RGBA8_UNorm_sRGB: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case TextureFormat::RGBA8_SNorm: return DXGI_FORMAT_R8G8B8A8_SNORM;
        case TextureFormat::RGBA8_UInt: return DXGI_FORMAT_R8G8B8A8_UINT;
        case TextureFormat::RGBA8_SInt: return DXGI_FORMAT_R8G8B8A8_SINT;
        case TextureFormat::RGBA16_Float: return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case TextureFormat::RGBA16_UInt: return DXGI_FORMAT_R16G16B16A16_UINT;
        case TextureFormat::RGBA16_SInt: return DXGI_FORMAT_R16G16B16A16_SINT;
        case TextureFormat::RGBA32_Float: return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case TextureFormat::RGBA32_UInt: return DXGI_FORMAT_R32G32B32A32_UINT;
        case TextureFormat::RGBA32_SInt: return DXGI_FORMAT_R32G32B32A32_SINT;
        case TextureFormat::BGRA8_UNorm: return DXGI_FORMAT_B8G8R8A8_UNORM;
        case TextureFormat::BGRA8_UNorm_sRGB: return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        case TextureFormat::D32_Float: return DXGI_FORMAT_D32_FLOAT;
        case TextureFormat::D24_UNorm_S8_UInt: return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case TextureFormat::D32_Float_S8_UInt: return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
        default: return DXGI_FORMAT_UNKNOWN;
    }
}

void DX12PipelineState::CreateInputLayout(std::vector<D3D12_INPUT_ELEMENT_DESC>& inputElements) const {
    inputElements.clear();
    inputElements.reserve(m_inputLayout.size());

    for (const auto& attr : m_inputLayout) {
        D3D12_INPUT_ELEMENT_DESC element = {};
        element.SemanticName = attr.semanticName.c_str();
        element.SemanticIndex = attr.semanticIndex;
        element.Format = GetDXGIFormat(static_cast<TextureFormat>(attr.format));
        element.InputSlot = attr.inputSlot;
        element.AlignedByteOffset = attr.alignedByteOffset;
        element.InputSlotClass = attr.instanceStepRate > 0 ? D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA : D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        element.InstanceDataStepRate = attr.instanceStepRate;

        inputElements.push_back(element);
    }
}

D3D12_ROOT_SIGNATURE_DESC1 DX12PipelineState::CreateRootSignatureDesc() const {
    D3D12_ROOT_SIGNATURE_DESC1 desc = {};
    desc.NumParameters = 0;
    desc.pParameters = nullptr;
    desc.NumStaticSamplers = 0;
    desc.pStaticSamplers = nullptr;
    desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // TODO: 根据着色器反射信息创建实际的根参数
    // 这里返回一个空的根签名作为占位符

    return desc;
}

uint64_t DX12PipelineState::CalculateCacheKey() const {
    uint64_t key = 0;

    // 包含着色器哈希
    for (const auto& shader : m_shaders) {
        if (shader) {
            key = key * 31 + shader->GetCompileHash();
        }
    }

    // 包含状态信息
    key = key * 31 + static_cast<uint64_t>(m_primitiveTopology);
    key = key * 31 + static_cast<uint64_t>(m_rasterizerState.fillMode);
    key = key * 31 + static_cast<uint64_t>(m_rasterizerState.cullMode);
    key = key * 31 + static_cast<uint64_t>(m_depthStencilState.depthFunc);
    key = key * 31 + static_cast<uint64_t>(m_sampleCount);

    // 包含格式信息
    for (const auto& format : m_renderTargetFormats) {
        key = key * 31 + static_cast<uint64_t>(format);
    }

    return key;
}

bool DX12PipelineState::ValidateGraphicsPipeline(std::string& errors) const {
    auto vertexShader = GetShader(ShaderType::Vertex);
    if (!vertexShader) {
        errors = "Graphics pipeline requires a vertex shader";
        return false;
    }

    if (m_renderTargetFormats.empty()) {
        errors = "Graphics pipeline requires at least one render target format";
        return false;
    }

    if (m_depthStencilFormat == TextureFormat::Unknown) {
        errors = "Graphics pipeline requires a depth stencil format";
        return false;
    }

    return true;
}

bool DX12PipelineState::ValidateComputePipeline(std::string& errors) const {
    auto computeShader = GetShader(ShaderType::Compute);
    if (!computeShader) {
        errors = "Compute pipeline requires a compute shader";
        return false;
    }

    return true;
}

} // namespace PrismaEngine::Graphic::DX12