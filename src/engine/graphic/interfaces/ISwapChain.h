#pragma once

#include "RenderTypes.h"
#include <memory>

namespace PrismaEngine::Graphic {

// 前置声明
class ITexture;
class IRenderDevice;

/// @brief 交换链模式
enum class SwapChainMode {
    Immediate,      // 立即呈现
    VSync,          // 垂直同步
    AdaptiveVSync,  // 自适应垂直同步
    TripleBuffer    // 三重缓冲
};

/// @brief 交换链抽象接口
class ISwapChain {
public:
    virtual ~ISwapChain() = default;

    /// @brief 获取缓冲区数量
    /// @return 缓冲区数量
    virtual uint32_t GetBufferCount() const = 0;

    /// @brief 获取当前缓冲区索引
    /// @return 当前缓冲区索引
    virtual uint32_t GetCurrentBufferIndex() const = 0;

    /// @brief 获取宽度
    /// @return 宽度
    virtual uint32_t GetWidth() const = 0;

    /// @brief 获取高度
    /// @return 高度
    virtual uint32_t GetHeight() const = 0;

    /// @brief 获取格式
    /// @return 格式
    virtual TextureFormat GetFormat() const = 0;

    /// @brief 获取模式
    /// @return 模式
    virtual SwapChainMode GetMode() const = 0;

    /// @brief 检查是否启用HDR
    /// @return 是否启用HDR
    virtual bool IsHDR() const = 0;

    // === 缓冲区访问 ===

    /// @brief 获取渲染目标
    /// @param bufferIndex 缓冲区索引
    /// @return 渲染目标纹理
    virtual ITexture* GetRenderTarget(uint32_t bufferIndex = 0) = 0;

    /// @brief 获取当前渲染目标
    /// @return 当前渲染目标纹理
    virtual ITexture* GetCurrentRenderTarget() = 0;

    // === 呈现控制 ===

    /// @brief 呈现缓冲区
    /// @return 是否成功
    virtual bool Present() = 0;

    /// @brief 设置模式
    /// @param mode 模式
    /// @return 是否成功
    virtual bool SetMode(SwapChainMode mode) = 0;

    /// @brief 调整大小
    /// @param width 新宽度
    /// @param height 新高度
    /// @return 是否成功
    virtual bool Resize(uint32_t width, uint32_t height) = 0;

    /// @brief 设置HDR
    /// @param enable 是否启用HDR
    /// @return 是否成功
    virtual bool SetHDR(bool enable) = 0;

    // === 颜色空间 ===

    /// @brief 获取颜色空间
    /// @return 颜色空间
    virtual const char* GetColorSpace() const = 0;

    /// @brief 设置颜色空间
    /// @param colorSpace 颜色空间名称
    /// @return 是否成功
    virtual bool SetColorSpace(const char* colorSpace) = 0;

    // === 统计信息 ===

    /// @brief 获取帧率
    /// @return 当前帧率
    virtual float GetFrameRate() const = 0;

    /// @brief 获取帧时间
    /// @return 最后一帧的时间（毫秒）
    virtual float GetFrameTime() const = 0;

    /// @brief 获取呈现统计
    struct PresentStats {
        uint32_t totalFrames = 0;
        uint32_t droppedFrames = 0;
        float averageFrameTime = 0.0f;
        float minFrameTime = FLT_MAX;
        float maxFrameTime = 0.0f;
        float executionTime    = 0.0f;
        float frameRate = 0.0f;
    };
    virtual PresentStats GetPresentStats() const = 0;

    /// @brief 重置统计信息
    virtual void ResetStats() = 0;

    // === 全屏控制 ===

    /// @brief 检查是否为全屏
    /// @return 是否为全屏
    virtual bool IsFullscreen() const = 0;

    /// @brief 设置全屏
    /// @param fullscreen 是否全屏
    /// @return 是否成功
    virtual bool SetFullscreen(bool fullscreen) = 0;

    // === 调试功能 ===

    /// @brief 截图到文件
    /// @param filename 文件名
    /// @param bufferIndex 缓冲区索引
    /// @return 是否成功
    virtual bool Screenshot(const std::string& filename, uint32_t bufferIndex = 0) = 0;

    /// @brief 启用调试层
    /// @param enable 是否启用
    virtual void EnableDebugLayer(bool enable) = 0;
};

} // namespace PrismaEngine::Graphic