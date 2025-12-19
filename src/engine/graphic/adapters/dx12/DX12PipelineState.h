#pragma once

#include <directx/d3d12.h>
#include <directx/d3dx12.h>

#include "interfaces/IPipelineState.h"
#include <wrl/client.h>

namespace PrismaEngine::Graphic::DX12 {

// 前置声明
class DX12RenderDevice;

/// @brief DirectX12管线状态对象适配器
/// 实现IPipelineState接口，包装ID3D12PipelineState
class DX12PipelineState : public IPipelineState {
public:
    /// @brief 构造函数
    /// @param device DirectX12渲染设备
    DX12PipelineState(DX12RenderDevice* device);

    /// @brief 析构函数
    ~DX12PipelineState() override;

    // IPipelineState接口实现
    PipelineType GetType() const override;
    bool IsValid() const override;

    // 着色器管理
    void SetShader(ShaderType type, std::shared_ptr<IShader> shader) override;
    std::shared_ptr<IShader> GetShader(ShaderType type) const override;
    bool HasShader(ShaderType type) const override;

    // 渲染状态
    void SetPrimitiveTopology(PrimitiveTopology topology) override;
    PrimitiveTopology GetPrimitiveTopology() const override;

    void SetBlendState(const BlendState& state, uint32_t renderTargetIndex) override;
    const BlendState& GetBlendState(uint32_t renderTargetIndex) const override;

    void SetRasterizerState(const RasterizerState& state) override;
    const RasterizerState& GetRasterizerState() const override;

    void SetDepthStencilState(const DepthStencilState& state) override;
    const DepthStencilState& GetDepthStencilState() const override;

    // 顶点输入布局
    void SetInputLayout(const std::vector<VertexInputAttribute>& attributes) override;
    const std::vector<VertexInputAttribute>& GetInputLayout() const override;
    uint32_t GetInputAttributeCount() const override;

    // 渲染目标格式
    void SetRenderTargetFormats(const std::vector<TextureFormat>& formats) override;
    void SetRenderTargetFormat(uint32_t index, TextureFormat format) override;
    TextureFormat GetRenderTargetFormat(uint32_t index) const override;
    uint32_t GetRenderTargetCount() const override;

    void SetDepthStencilFormat(TextureFormat format) override;
    TextureFormat GetDepthStencilFormat() const override;

    // 多重采样
    void SetSampleCount(uint32_t sampleCount, uint32_t sampleQuality) override;
    uint32_t GetSampleCount() const override;
    uint32_t GetSampleQuality() const override;

    // 创建和验证
    bool Create(IRenderDevice* device) override;
    bool Recreate() override;
    bool Validate(IRenderDevice* device, std::string& errors) const override;

    // 缓存
    uint64_t GetCacheKey() const override;
    bool LoadFromCache(IRenderDevice* device, uint64_t cacheKey) override;
    bool SaveToCache() const override;

    // 调试
    const std::string& GetErrors() const override;
    void SetDebugName(const std::string& name) override;
    const std::string& GetDebugName() const override;

    // 克隆
    std::unique_ptr<IPipelineState> Clone() const override;

    // === DirectX12特定方法 ===

    /// @brief 获取管线状态对象
    /// @return PSO指针
    ID3D12PipelineState* GetPipelineState() const { return m_pipelineState.Get(); }

    /// @brief 获取根签名
    /// @return 根签名指针
    ID3D12RootSignature* GetRootSignature() const { return m_rootSignature.Get(); }

    /// @brief 创建D3D12管线状态描述
    D3D12_GRAPHICS_PIPELINE_STATE_DESC CreateD3D12PipelineDesc() const;

    /// @brief 创建D3D12计算管线状态描述
    D3D12_COMPUTE_PIPELINE_STATE_DESC CreateD3D12ComputePipelineDesc() const;

    /// @brief 创建D3D12根签名
    bool CreateD3D12RootSignature();

    /// @brief 是否为计算管线
    bool IsComputePipeline() const { return m_type == PipelineType::Compute; }

private:
    DX12RenderDevice* m_device;

    // DirectX12对象
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;

    // 着色器存储
    std::array<std::shared_ptr<IShader>, 6> m_shaders;  // Vertex, Pixel, Geometry, Hull, Domain, Compute

    // 混合状态（支持多个渲染目标）
    std::vector<BlendState> m_blendStates;

    // 顶点输入布局
    std::vector<VertexInputAttribute> m_inputLayout;

    // 光栅化状态
    RasterizerState m_rasterizerState;

    // 缓存相关
    bool m_cacheEnabled = true;
    uint64_t m_cacheKey = 0;

    // 辅助方法
    D3D12_PRIMITIVE_TOPOLOGY_TYPE GetD3D12PrimitiveTopology() const;
    D3D12_FILL_MODE GetD3D12FillMode() const;
    D3D12_CULL_MODE GetD3D12CullMode() const;
    D3D12_COMPARISON_FUNC GetD3D12ComparisonFunc(ComparisonFunc func) const;
    D3D12_BLEND_OP GetD3D12BlendOp(BlendOp op) const;
    D3D12_BLEND GetD3D12Blend(BlendFactorType factor) const;
    DXGI_FORMAT GetDXGIFormat(TextureFormat format) const;
    void CreateInputLayout(std::vector<D3D12_INPUT_ELEMENT_DESC>& inputElements) const;
    D3D12_ROOT_SIGNATURE_DESC1 CreateRootSignatureDesc() const;

    // 计算缓存键
    uint64_t CalculateCacheKey() const;

    // 验证方法
    bool ValidateGraphicsPipeline(std::string& errors) const;
    bool ValidateComputePipeline(std::string& errors) const;
};

} // namespace PrismaEngine::Graphic::DX12