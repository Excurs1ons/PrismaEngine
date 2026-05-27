#pragma once

#include "RenderTypes.h"

namespace Prisma::Graphic {

// 前置声明
class ITexture;
class IRenderDevice;

// 交换链抽象接口
class ISwapChain {
public:
    virtual ~ISwapChain() = default;

    // 获取缓冲区数量
    [[nodiscard]] virtual uint32_t GetBufferCount() const = 0;

    // 获取当前缓冲区索引
    [[nodiscard]] virtual uint32_t GetCurrentBufferIndex() const = 0;

    // 获取宽度
    [[nodiscard]] virtual uint32_t GetWidth() const = 0;

    // 获取高度
    [[nodiscard]] virtual uint32_t GetHeight() const = 0;

    // 获取格式
    [[nodiscard]] virtual TextureFormat GetFormat() const = 0;

    // 获取模式
    [[nodiscard]] virtual PresentMode GetMode() const = 0;

    // 检查是否启用HDR
    [[nodiscard]] virtual bool IsHDR() const = 0;

    // === 缓冲区访问 ===

    // 获取渲染目标
    virtual ITexture* GetRenderTarget(uint32_t bufferIndex = 0) = 0;

    // 获取当前渲染目标
    virtual ITexture* GetCurrentRenderTarget() = 0;

    // === 呈现控制 ===

    // 呈现缓冲区
    virtual bool Present() = 0;

    // 设置模式
    virtual bool SetMode(PresentMode mode) = 0;

    // 调整大小
    virtual bool Resize(uint32_t width, uint32_t height) = 0;

    // 设置HDR
    virtual bool SetHDR(bool enable) = 0;

    // === 颜色空间 ===

    // 获取颜色空间
    [[nodiscard]] virtual const char* GetColorSpace() const = 0;

    // 设置颜色空间
    virtual bool SetColorSpace(const char* colorSpace) = 0;

    // === 统计信息 ===

    // 获取帧率
    [[nodiscard]] virtual float GetFrameRate() const = 0;

    // 获取帧时间
    [[nodiscard]] virtual float GetFrameTime() const = 0;

    // 获取呈现统计
    struct PresentStats {
        uint32_t totalFrames = 0;
        uint32_t droppedFrames = 0;
        float averageFrameTime = 0.0f;
        float minFrameTime = FLT_MAX;
        float maxFrameTime = 0.0f;
        float executionTime    = 0.0f;
        float frameRate = 0.0f;
    };
    [[nodiscard]] virtual PresentStats GetPresentStats() const = 0;

    // 重置统计信息
    virtual void ResetStats() = 0;

    // === 全屏控制 ===

    // 检查是否为全屏
    [[nodiscard]] virtual bool IsFullscreen() const = 0;

    // 设置全屏
    virtual bool SetFullscreen(bool fullscreen) = 0;

    // === 调试功能 ===

    // 截图到文件
    virtual bool Screenshot(const std::string& filename, uint32_t bufferIndex = 0) = 0;

    // 启用调试层
    virtual void EnableDebugLayer(bool enable) = 0;
};

} // namespace Prisma::Graphic