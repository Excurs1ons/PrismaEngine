#pragma once

#include "RenderTypes.h"
#include <cstdint>
#include <memory>

namespace PrismaEngine::Graphic {

/// @brief 渲染目标抽象接口
/// 代表颜色附着点，可以是纹理、交换链等
class IRenderTarget {
public:
    virtual ~IRenderTarget() = default;

    /// @brief 获取渲染目标宽度
    virtual uint32_t GetWidth() const = 0;

    /// @brief 获取渲染目标高度
    virtual uint32_t GetHeight() const = 0;

    /// @brief 获取渲染目标格式
    virtual TextureFormat GetFormat() const = 0;

    /// @brief 获取渲染目标类型
    virtual TextureType GetType() const = 0;

    /// @brief 获取原生句柄（用于后端适配）
    virtual void* GetNativeHandle() const = 0;

    /// @brief 是否为交换链
    virtual bool IsSwapChain() const = 0;

    /// @brief 清除渲染目标
    /// @param color 清除颜色 (RGBA)
    virtual void Clear(const float color[4]) = 0;
};

/// @brief 深度模板缓冲区抽象接口
/// 代表深度模板附着点
class IDepthStencil {
public:
    virtual ~IDepthStencil() = default;

    /// @brief 获取深度模板宽度
    virtual uint32_t GetWidth() const = 0;

    /// @brief 获取深度模板高度
    virtual uint32_t GetHeight() const = 0;

    /// @brief 获取深度格式
    virtual TextureFormat GetFormat() const = 0;

    /// @brief 获取原生句柄（用于后端适配）
    virtual void* GetNativeHandle() const = 0;

    /// @brief 清除深度
    /// @param depth 深度值（0.0 - 1.0）
    virtual void ClearDepth(float depth) = 0;

    /// @brief 清除模板
    /// @param stencil 模板值
    virtual void ClearStencil(uint8_t stencil) = 0;

    /// @brief 清除深度和模板
    /// @param depth 深度值（0.0 - 1.0）
    /// @param stencil 模板值
    virtual void Clear(float depth, uint8_t stencil) = 0;
};

/// @brief 纹理渲染目标视图抽象接口
/// 继承自 IRenderTarget，提供纹理特定的功能
class ITextureRenderTarget : public IRenderTarget {
public:
    virtual ~ITextureRenderTarget() = default;

    /// @brief 获取 mip 级数
    virtual uint32_t GetMipLevels() const = 0;

    /// @brief 获取数组大小
    virtual uint32_t GetArraySize() const = 0;

    /// @brief 获取纹理资源
    virtual ITexture* GetTexture() = 0;
};

/// @brief 交换链渲染目标抽象接口
/// 继承自 IRenderTarget，提供交换链特定的功能
class ISwapChainRenderTarget : public IRenderTarget {
public:
    virtual ~ISwapChainRenderTarget() = default;

    /// @brief 获取当前后缓冲索引
    virtual uint32_t GetCurrentBackBufferIndex() const = 0;

    /// @brief 获取缓冲数量
    virtual uint32_t GetBufferCount() const = 0;

    /// @brief 是否为交换链
    virtual bool IsSwapChain() const override { return true; }
};

} // namespace PrismaEngine::Graphic
