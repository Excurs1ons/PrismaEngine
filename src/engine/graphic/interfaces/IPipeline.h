#pragma once

#include "IResource.h"
#include "RenderTypes.h"
#include <memory>

namespace PrismaEngine::Graphic {

// 前置声明
class IShader;
class IRenderDevice;
class IBuffer;
class ITexture;

/// @brief 管线状态
enum class PipelineState {
    Idle,
    Building,
    Ready,
    Failed
};

/// @brief 图元拓扑类型
enum class PrimitiveTopology {
    Undefined = 0,
    PointList = 1,
    LineList = 2,
    LineStrip = 3,
    TriangleList = 4,
    TriangleStrip = 5
};

/// @brief 混合操作
enum class BlendOp {
    Add = 1,
    Subtract = 2,
    RevSubtract = 3,
    Min = 4,
    Max = 5
};

/// @brief 混合因子
enum class BlendFactor {
    Zero = 0,
    One = 1,
    SrcColor = 2,
    InvSrcColor = 3,
    SrcAlpha = 4,
    InvSrcAlpha = 5,
    DestAlpha = 6,
    InvDestAlpha = 7,
    DestColor = 8,
    InvDestColor = 9,
    SrcAlphaSat = 10,
    BlendFactor = 11,
    InvBlendFactor = 12,
    Src1Color = 13,
    InvSrc1Color = 14,
    Src1Alpha = 15,
    InvSrc1Alpha = 16
};

/// @brief 填充模式
enum class FillMode {
    Solid = 0,
    Wireframe = 1
};

/// @brief 裁剪模式
enum class CullMode {
    None = 1,
    Front = 2,
    Back = 3
};

/// @brief 深度比较函数
enum class ComparisonFunc {
    Never = 1,
    Less = 2,
    Equal = 3,
    LessEqual = 4,
    Greater = 5,
    NotEqual = 6,
    GreaterEqual = 7,
    Always = 8
};

/// @/// @brief 模板操作
enum class StencilOp {
    Keep = 1,
    Zero = 2,
    Replace = 3,
    IncreaseSat = 4,
    DecreaseSat = 5,
    Invert = 6,
    Increase = 7,
    Decrease = 8
};

/// @brief 渲染管线抽象接口
class IPipeline : public IResource {
public:
    virtual ~IPipeline() = default;

    /// @brief 获取管线状态
    /// @return 管线状态
    virtual PipelineState GetState() const = 0;

    /// @brief 获取图元拓扑类型
    /// @return 图元拓扑类型
    virtual PrimitiveTopology GetPrimitiveTopology() const = 0;

    // === 着色器访问 ===

    /// @brief 获取顶点着色器
    /// @return 顶点着色器智能指针
    virtual std::shared_ptr<IShader> GetVertexShader() const = 0;

    /// @brief 获取像素着色器
    /// @return 像素着色器智能指针
    virtual std::shared_ptr<IShader> GetPixelShader() const = 0;

    /// @brief 获取几何着色器
    /// @return 几何着色器智能指针
    virtual std::shared_ptr<IShader> GetGeometryShader() const = 0;

    /// @brief 获取外壳着色器
    /// @return 外壳着色器智能指针
    virtual std::shared_ptr<IShader> GetHullShader() const = 0;

    /// @brief 获取域着色器
    /// @return 域着色器智能指针
    virtual std::shared_ptr<IShader> GetDomainShader() const = 0;

    /// @brief 获取计算着色器
    /// @return 计算着色器智能指针
    virtual std::shared_ptr<IShader> GetComputeShader() const = 0;

    // === 渲染目标信息 ===

    /// @brief 获取渲染目标数量
    /// @return 渲染目标数量
    virtual uint32_t GetRenderTargetCount() const = 0;

    /// @brief 获取渲染目标格式
    /// @param index 渲染目标索引
    /// @return 渲染目标格式
    virtual TextureFormat GetRenderTargetFormat(uint32_t index) const = 0;

    /// @brief 获取深度模板格式
    /// @return 深度模板格式
    virtual TextureFormat GetDepthStencilFormat() const = 0;

    /// @brief 获取采样数量
    /// @return 采样数量
    virtual uint32_t GetSampleCount() const = 0;

    /// @brief 获取采样质量
    /// @return 采样质量
    virtual uint32_t GetSampleQuality() const = 0;

    // === 状态查询 ===

    /// @brief 检查是否启用深度测试
    /// @return 是否启用深度测试
    virtual bool IsDepthTestEnabled() const = 0;

    /// @brief 检查是否启用深度写入
    /// @return 是否启用深度写入
    virtual bool IsDepthWriteEnabled() const = 0;

    /// @brief 获取深度比较函数
    /// @return 深度比较函数
    virtual ComparisonFunc GetDepthFunc() const = 0;

    /// @brief 检查是否启用模板测试
    /// @return 是否启用模板测试
    virtual bool IsStencilEnabled() const = 0;

    /// @brief 检查是否启用裁剪
    /// @return 是否启用裁剪
    virtual bool IsCullingEnabled() const = 0;

    /// @brief 获取裁剪模式
    /// @return 裁剪模式
    virtual CullMode GetCullMode() const = 0;

    /// @brief 获取填充模式
    /// @return 填充模式
    virtual FillMode GetFillMode() const = 0;

    /// @brief 检查是否启用混合
    /// @return 是否启用混合
    virtual bool IsBlendingEnabled() const = 0;

    /// @brief 获取混合操作
    /// @return 混合操作
    virtual BlendOp GetBlendOp() const = 0;

    /// @brief 获取源混合因子
    /// @return 源混合因子
    virtual BlendFactor GetSrcBlendFactor() const = 0;

    /// @brief 获取目标混合因子
    /// @return 目标混合因子
    virtual BlendFactor GetDestBlendFactor() const = 0;

    // === 顶点输入布局 ===

    /// @brief 获取顶点输入元素数量
    /// @return 顶点输入元素数量
    virtual uint32_t GetVertexInputElementCount() const = 0;

    /// @brief 获取顶点输入元素
    /// @param index 元素索引
    /// @param[out] semanticName 语义名称
    /// @param[out] semanticIndex 语义索引
    /// @param[out] format 格式
    /// @param[out] inputSlot 输入槽
    /// @param[out] alignedByteOffset 对齐字节偏移
    /// @param[out] inputSlotClass 输入槽类型
    /// @param[out] instanceDataStepRate 实例数据步率
    virtual void GetVertexInputElement(uint32_t index,
                                       std::string& semanticName,
                                       uint32_t& semanticIndex,
                                       TextureFormat& format,
                                       uint32_t& inputSlot,
                                       uint32_t& alignedByteOffset,
                                       uint32_t& inputSlotClass,
                                       uint32_t& instanceDataStepRate) const = 0;

    // === 管线修改 ===

    /// @brief 设置着色器
    /// @param shader 着色器智能指针
    /// @return 是否成功
    virtual bool SetShader(std::shared_ptr<IShader> shader) = 0;

    /// @brief 设置渲染目标格式
    /// @param index 渲染目标索引
    /// @param format 格式
    /// @return 是否成功
    virtual bool SetRenderTargetFormat(uint32_t index, TextureFormat format) = 0;

    /// @brief 设置深度模板格式
    /// @param format 格式
    /// @return 是否成功
    virtual bool SetDepthStencilFormat(TextureFormat format) = 0;

    /// @brief 设置采样数量
    /// @param sampleCount 采样数量
    /// @return 是否成功
    virtual bool SetSampleCount(uint32_t sampleCount) = 0;

    /// @brief 设置深度状态
    /// @param enable 是否启用深度测试
    /// @param write 是否启用深度写入
    /// @param func 比较函数
    /// @return 是否成功
    virtual bool SetDepthState(bool enable, bool write, ComparisonFunc func) = 0;

    /// @brief 设置模板状态
    /// @param enable 是否启用模板测试
    /// @param readMask 读取掩码
    /// @param writeMask 写入掩码
    /// @return 是否成功
    virtual bool SetStencilState(bool enable, uint8_t readMask, uint8_t writeMask) = 0;

    /// @brief 设置裁剪状态
    /// @param enable 是否启用裁剪
    /// @param mode 裁剪模式
    /// @param frontCCW 是否正面逆时针
    /// @return 是否成功
    virtual bool SetRasterizerState(bool enable, CullMode mode, bool frontCCW) = 0;

    /// @brief 设置填充模式
    /// @param mode 填充模式
    /// @return 是否成功
    virtual bool SetFillMode(FillMode mode) = 0;

    /// @brief 设置混合状态
    /// @param enable 是否启用混合
    /// @param srcBlend 源混合因子
    /// @param destBlend 目标混合因子
    /// @param blendOp 混合操作
    /// @param srcBlendAlpha 源Alpha混合因子
    /// @param destBlendAlpha 目标Alpha混合因子
    /// @param blendOpAlpha Alpha混合操作
    /// @return 是否成功
    virtual bool SetBlendState(bool enable,
                               BlendFactor srcBlend, BlendFactor destBlend, BlendOp blendOp,
                               BlendFactor srcBlendAlpha, BlendFactor destBlendAlpha, BlendOp blendOpAlpha) = 0;

    // === 管线构建 ===

    /// @brief 构建管线
    /// @return 是否构建成功
    virtual bool Build() = 0;

    /// @brief 重新构建管线（在修改后调用）
    /// @return 是否重新构建成功
    virtual bool Rebuild() = 0;

    /// @brief 验证管线配置
    /// @return 是否有效
    virtual bool Validate() = 0;

    /// @brief 获取构建错误信息
    /// @return 错误信息
    virtual const std::string& GetBuildErrors() const = 0;

    // === 管线克隆 ===

    /// @brief 克隆管线
    /// @return 克隆的管线智能指针
    virtual std::shared_ptr<IPipeline> Clone() const = 0;

    // === 调试功能 ===

    /// @brief 打印管线信息
    virtual void DebugPrintInfo() const = 0;

    /// @brief 导出管线配置到文件
    /// @param filename 文件名
    /// @return 是否成功
    virtual bool DebugExportToFile(const std::string& filename) const = 0;

    // === 管线缓存 ===

    /// @brief 启用管线缓存
    /// @param enable 是否启用
    virtual void EnableCache(bool enable) = 0;

    /// @brief 获取管线缓存键
    /// @return 缓存键
    virtual uint64_t GetCacheKey() const = 0;

    /// @brief 从缓存加载管线
    /// @param cacheKey 缓存键
    /// @return 是否加载成功
    virtual bool LoadFromCache(uint64_t cacheKey) = 0;

    /// @brief 保存管线到缓存
    /// @return 是否保存成功
    virtual bool SaveToCache() const = 0;
};

} // namespace PrismaEngine::Graphic