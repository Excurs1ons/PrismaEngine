#pragma once

#include "IRenderTarget.h"
#include <memory>

namespace Prisma::Graphic {

class IDeviceContext;

// G-Buffer 目标枚举
/// 延迟渲染的几何缓冲区目标
enum class GBufferTarget : uint32_t {
    Position = 0,    // RGB: 位置 + A: 粗糙度
    Normal = 1,      // RGB: 法线 + A: 金属度
    Albedo = 2,      // RGB: 颜色 + A: 环境光遮蔽
    Emissive = 3,    // RGB: 自发光 + A: 材质 ID
    Depth = 4,       // 深度缓冲
    Count
};

// G-Buffer 抽象接口
/// 延迟渲染的几何缓冲区，包含多个渲染目标
class IGBuffer {
public:
    virtual ~IGBuffer() = default;

    // 初始化 G-Buffer
    virtual bool Initialize(uint32_t width, uint32_t height) = 0;

    // 调整大小
    virtual bool Resize(uint32_t width, uint32_t height) = 0;

    // 获取宽度
    virtual uint32_t GetWidth() const = 0;

    // 获取高度
    virtual uint32_t GetHeight() const = 0;

    // 是否已初始化
    virtual bool IsInitialized() const = 0;

    // === 渲染目标访问 ===

    // 获取颜色渲染目标
    virtual ITextureRenderTarget* GetTarget(GBufferTarget target) = 0;

    // 获取深度模板
    virtual IDepthStencil* GetDepthStencil() = 0;

    // 获取所有颜色渲染目标
    virtual void GetColorTargets(ITextureRenderTarget** targets, uint32_t count) = 0;

    // 获取颜色渲染目标数量
    virtual uint32_t GetColorTargetCount() const = 0;

    // === 着色器资源访问 ===

    // 设置为着色器资源（用于光照 Pass）
    virtual void BindAsShaderResources(IDeviceContext* deviceContext, uint32_t startSlot = 0) = 0;

    // 取消绑定着色器资源
    virtual void UnbindShaderResources(IDeviceContext* deviceContext, uint32_t startSlot = 0, uint32_t count = 4) = 0;

    // === 清除操作 ===

    // 清除所有渲染目标
    virtual void Clear(IDeviceContext* deviceContext, const float color[4]) = 0;

    // 清除深度
    virtual void ClearDepth(IDeviceContext* deviceContext, float depth = 1.0f) = 0;

    // 获取目标格式
    virtual TextureFormat GetTargetFormat(GBufferTarget target) const = 0;

    // 设置为渲染目标
    virtual void SetAsRenderTarget(IDeviceContext* deviceContext) = 0;
};

} // namespace Prisma::Graphic
