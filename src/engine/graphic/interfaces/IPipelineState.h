#pragma once

#include "RenderTypes.h"
#include "IShader.h"
#include <memory>
#include <vector>
#include "IRenderDevice.h"
#define INT int
#ifdef _WIN32
#define BOOL int
#else
#define BOOL bool
#endif
namespace PrismaEngine::Graphic {

/// @brief 混合状态描述
struct BlendState {
    BOOL blendEnable = false;
    BOOL logicOpEnable = false;
    uint32_t writeMask = 0xF;  // RGBA all enabled
    BlendOp blendOp = BlendOp::Add;
    BlendFactorType srcBlend = BlendFactorType::One;
    BlendFactorType destBlend = BlendFactorType::Zero;
    BlendOp blendOpAlpha = BlendOp::Add;
    BlendFactorType srcBlendAlpha = BlendFactorType::One;
    BlendFactorType destBlendAlpha = BlendFactorType::Zero;

    static const BlendState Default;
};

/// @brief 光栅化器状态描述
struct RasterizerState {
    BOOL cullEnable = true;
    BOOL frontCounterClockwise = false;
    BOOL depthClipEnable = true;
    BOOL scissorEnable = false;
    BOOL multisampleEnable = false;
    BOOL antialiasedLineEnable = false;
    bool conservativeRaster = false;
    FillMode fillMode = FillMode::Solid;
    CullMode cullMode = CullMode::Back;
    INT depthBias = 0;
    float depthBiasClamp = 0.0f;
    float slopeScaledDepthBias = 0.0f;

    static const RasterizerState Default;
};

/// @brief 深度模板状态描述
struct DepthStencilState {
    BOOL depthEnable = true;
    bool depthWriteEnable = true;
    BOOL stencilEnable = false;
    ComparisonFunc depthFunc = ComparisonFunc::Less;

    // 模板操作
    struct StencilOpDesc {
        StencilOp failOp = StencilOp::Keep;
        StencilOp depthFailOp = StencilOp::Keep;
        StencilOp passOp = StencilOp::Keep;
        ComparisonFunc func = ComparisonFunc::Always;
    };

    StencilOpDesc frontFace;
    StencilOpDesc backFace;
    uint8_t stencilReadMask = 0xFF;
    uint8_t stencilWriteMask = 0xFF;
    uint8_t stencilRef = 0;

    static const DepthStencilState Default;
};

/// @brief 顶点输入属性
struct VertexInputAttribute {
    std::string semanticName;
    uint32_t semanticIndex = 0;
    TextureFormat format = TextureFormat::RGBA32_Float;
    uint32_t inputSlot = 0;
    uint32_t alignedByteOffset = 0xFFFFFFFF;  // D3D12_APPEND_ALIGNED_ELEMENT
    uint32_t inputSlotClass = 0;  // 0=vertex, 1=instance
    uint32_t instanceDataStepRate = 0;
};

/// @brief 管线状态抽象接口
/// 代表一个编译好的渲染管线状态对象(PSO)
class IPipelineState {
public:
    virtual ~IPipelineState() = default;

    /// @brief 获取管线类型
    /// @return 管线类型
    [[nodiscard]] virtual PipelineType GetType() const = 0;

    /// @brief 获取管线状态
    /// @return 是否已创建/有效
    [[nodiscard]] virtual bool IsValid() const = 0;

    // === 着色器管理 ===

    /// @brief 设置着色器
    /// @param type 着色器类型
    /// @param shader 着色器对象
    virtual void SetShader(ShaderType type, std::shared_ptr<IShader> shader) = 0;

    /// @brief 获取着色器
    /// @param type 着色器类型
    /// @return 着色器对象
    [[nodiscard]] virtual std::shared_ptr<IShader> GetShader(ShaderType type) const = 0;

    /// @brief 检查是否有指定类型的着色器
    /// @param type 着色器类型
    /// @return 是否存在
    [[nodiscard]] virtual bool HasShader(ShaderType type) const = 0;

    // === 渲染状态 ===

    /// @brief 设置图元拓扑
    /// @param topology 图元拓扑
    virtual void SetPrimitiveTopology(PrimitiveTopology topology) = 0;

    /// @brief 获取图元拓扑
    /// @return 图元拓扑
    [[nodiscard]] virtual PrimitiveTopology GetPrimitiveTopology() const = 0;

    /// @brief 设置混合状态
    /// @param state 混合状态
    /// @param renderTargetIndex 渲染目标索引（如果需要独立设置）
    virtual void SetBlendState(const BlendState& state, uint32_t renderTargetIndex = 0) = 0;

    /// @brief 获取混合状态
    /// @param renderTargetIndex 渲染目标索引
    /// @return 混合状态
    [[nodiscard]] virtual const BlendState& GetBlendState(uint32_t renderTargetIndex = 0) const = 0;

    /// @brief 设置光栅化器状态
    /// @param state 光栅化器状态
    virtual void SetRasterizerState(const RasterizerState& state) = 0;

    /// @brief 获取光栅化器状态
    /// @return 光栅化器状态
    [[nodiscard]] virtual const RasterizerState& GetRasterizerState() const = 0;

    /// @brief 设置深度模板状态
    /// @param state 深度模板状态
    virtual void SetDepthStencilState(const DepthStencilState& state) = 0;

    /// @brief 获取深度模板状态
    /// @return 深度模板状态
    [[nodiscard]] virtual const DepthStencilState& GetDepthStencilState() const = 0;

    // === 顶点输入 ===

    /// @brief 设置顶点输入布局
    /// @param attributes 输入属性数组
    virtual void SetInputLayout(const std::vector<VertexInputAttribute>& attributes) = 0;

    /// @brief 获取顶点输入布局
    /// @return 输入属性数组
    [[nodiscard]] virtual const std::vector<VertexInputAttribute>& GetInputLayout() const = 0;

    /// @brief 获取输入属性数量
    /// @return 属性数量
    [[nodiscard]] virtual uint32_t GetInputAttributeCount() const = 0;

    // === 渲染目标 ===

    /// @brief 设置渲染目标格式
    /// @param formats 格式数组
    virtual void SetRenderTargetFormats(const std::vector<TextureFormat>& formats) = 0;

    /// @brief 设置单个渲染目标格式
    /// @param index 渲染目标索引
    /// @param format 格式
    virtual void SetRenderTargetFormat(uint32_t index, TextureFormat format) = 0;

    /// @brief 获取渲染目标格式
    /// @param index 渲染目标索引
    /// @return 格式
    [[nodiscard]] virtual TextureFormat GetRenderTargetFormat(uint32_t index) const = 0;

    /// @brief 获取渲染目标数量
    /// @return 数量
    [[nodiscard]] virtual uint32_t GetRenderTargetCount() const = 0;

    /// @brief 设置深度模板格式
    /// @param format 格式
    virtual void SetDepthStencilFormat(TextureFormat format) = 0;

    /// @brief 获取深度模板格式
    /// @return 格式
    [[nodiscard]] virtual TextureFormat GetDepthStencilFormat() const = 0;

    // === 多重采样 ===

    /// @brief 设置多重采样参数
    /// @param sampleCount 采样数
    /// @param sampleQuality 采样质量
    virtual void SetSampleCount(uint32_t sampleCount, uint32_t sampleQuality = 0) = 0;

    /// @brief 获取采样数
    /// @return 采样数
    [[nodiscard]] virtual uint32_t GetSampleCount() const = 0;

    /// @brief 获取采样质量
    /// @return 采样质量
    [[nodiscard]] virtual uint32_t GetSampleQuality() const = 0;

    // === 缓存和编译 ===

    /// @brief 创建/编译管线状态对象
    /// @param device 渲染设备
    /// @return 是否成功
    virtual bool Create(IRenderDevice* device) = 0;

    /// @brief 重新创建（在修改状态后）
    /// @return 是否成功
    virtual bool Recreate() = 0;

    /// @brief 验证配置是否有效
    /// @param device 渲染设备
    /// @param[out] errors 错误信息
    /// @return 是否有效
    virtual bool Validate(IRenderDevice* device, std::string& errors) const = 0;

    // === 缓存 ===

    /// @brief 获取管线缓存键
    /// @return 缓存键
    virtual uint64_t GetCacheKey() const = 0;

    /// @brief 从缓存加载
    /// @param device 渲染设备
    /// @param cacheKey 缓存键
    /// @return 是否加载成功
    virtual bool LoadFromCache(IRenderDevice* device, uint64_t cacheKey) = 0;

    /// @brief 保存到缓存
    /// @return 是否保存成功
    virtual bool SaveToCache() const = 0;

    // === 调试 ===

    /// @brief 获取创建错误信息
    /// @return 错误信息
    virtual const std::string& GetErrors() const = 0;

    /// @brief 设置调试名称
    /// @param name 名称
    virtual void SetDebugName(const std::string& name) = 0;

    /// @brief 获取调试名称
    /// @return 名称
    virtual const std::string& GetDebugName() const = 0;

    // === 克隆 ===

    /// @brief 克隆管线状态
    /// @return 新的管线状态对象
    virtual std::unique_ptr<IPipelineState> Clone() const = 0;

protected:
    PipelineType m_type = PipelineType::Graphics;
    PrimitiveTopology m_topology = PrimitiveTopology::TriangleList;
    BlendState m_blendState;
    RasterizerState m_rasterizerState;
    DepthStencilState m_depthStencilState;
    std::vector<VertexInputAttribute> m_inputAttributes;
    std::vector<TextureFormat> m_renderTargetFormats;
    TextureFormat m_depthStencilFormat = TextureFormat::D32_Float;
    uint32_t m_sampleCount = 1;
    uint32_t m_sampleQuality = 0;
    std::string m_debugName;
    std::string m_errors;
    bool m_isValid = false;
};

} // namespace PrismaEngine::Graphic