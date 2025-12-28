#pragma once

#include "IRenderTarget.h"
#include <memory>

namespace PrismaEngine::Graphic {

/// @brief G-Buffer 目标枚举
/// 延迟渲染的几何缓冲区目标
enum class GBufferTarget : uint32_t {
    Position = 0,    // RGB: 位置 + A: 粗糙度
    Normal = 1,      // RGB: 法线 + A: 金属度
    Albedo = 2,      // RGB: 颜色 + A: 环境光遮蔽
    Emissive = 3,    // RGB: 自发光 + A: 材质 ID
    Depth = 4,       // 深度缓冲
    Count
};

/// @brief G-Buffer 抽象接口
/// 延迟渲染的几何缓冲区，包含多个渲染目标
class IGBuffer {
public:
    virtual ~IGBuffer() = default;

    /// @brief 初始化 G-Buffer
    /// @param width 宽度
    /// @param height 高度
    /// @return 是否初始化成功
    virtual bool Initialize(uint32_t width, uint32_t height) = 0;

    /// @brief 调整大小
    /// @param width 新宽度
    /// @param height 新高度
    /// @return 是否调整成功
    virtual bool Resize(uint32_t width, uint32_t height) = 0;

    /// @brief 获取宽度
    virtual uint32_t GetWidth() const = 0;

    /// @brief 获取高度
    virtual uint32_t GetHeight() const = 0;

    /// @brief 是否已初始化
    virtual bool IsInitialized() const = 0;

    // === 渲染目标访问 ===

    /// @brief 获取颜色渲染目标
    /// @param target 目标类型
    /// @return 渲染目标接口指针
    virtual ITextureRenderTarget* GetTarget(GBufferTarget target) = 0;

    /// @brief 获取深度模板
    /// @return 深度模板接口指针
    virtual IDepthStencil* GetDepthStencil() = 0;

    /// @brief 获取所有颜色渲染目标
    /// @param targets 输出数组（至少 4 个元素）
    /// @param count 数组大小
    virtual void GetColorTargets(ITextureRenderTarget** targets, uint32_t count) = 0;

    /// @brief 获取颜色渲染目标数量
    virtual uint32_t GetColorTargetCount() const = 0;

    // === 着色器资源访问 ===

    /// @brief 设置为着色器资源（用于光照 Pass）
    /// @param deviceContext 设备上下文
    /// @param startSlot 起始槽位
    virtual void BindAsShaderResources(IDeviceContext* deviceContext, uint32_t startSlot = 0) = 0;

    /// @brief 取消绑定着色器资源
    /// @param deviceContext 设备上下文
    /// @param startSlot 起始槽位
    /// @param count 槽位数量
    virtual void UnbindShaderResources(IDeviceContext* deviceContext, uint32_t startSlot = 0, uint32_t count = 4) = 0;

    // === 清除操作 ===

    /// @brief 清除所有渲染目标
    /// @param deviceContext 设备上下文
    /// @param color 清除颜色
    virtual void Clear(IDeviceContext* deviceContext, const float color[4]) = 0;

    /// @brief 清除深度
    /// @param deviceContext 设备上下文
    /// @param depth 深度值
    virtual void ClearDepth(IDeviceContext* deviceContext, float depth = 1.0f) = 0;

    /// @brief 获取目标格式
    /// @param target 目标类型
    /// @return 纹理格式
    virtual TextureFormat GetTargetFormat(GBufferTarget target) const = 0;

    /// @brief 设置为渲染目标
    /// @param deviceContext 设备上下文
    virtual void SetAsRenderTarget(IDeviceContext* deviceContext) = 0;
};

} // namespace PrismaEngine::Graphic
