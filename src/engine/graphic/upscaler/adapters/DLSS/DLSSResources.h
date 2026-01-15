#pragma once

#include "interfaces/ITexture.h"
#include "interfaces/IBuffer.h"
#include <memory>
#include <vector>

namespace PrismaEngine::Graphic {

// 前置声明
class IRenderDevice;

/// @brief DLSS 4.5 GPU 资源管理器
/// 管理 DLSS 超分辨率所需的所有 GPU 资源
class DLSSResources {
public:
    DLSSResources();
    ~DLSSResources();

    /// @brief 初始化 DLSS 资源
    /// @param device 渲染设备
    /// @param renderWidth 渲染宽度
    /// @param renderHeight 渲染高度
    /// @param displayWidth 显示宽度
    /// @param displayHeight 显示高度
    /// @param maxFramesInFlight 最大帧数
    /// @return 是否初始化成功
    bool Initialize(IRenderDevice* device,
                    uint32_t renderWidth,
                    uint32_t renderHeight,
                    uint32_t displayWidth,
                    uint32_t displayHeight,
                    uint32_t maxFramesInFlight);

    /// @brief 释放所有资源
    void Release();

    /// @brief 检查是否已初始化
    bool IsInitialized() const { return m_initialized; }

    // === 输入资源访问 ===

    /// @brief 获取当前帧颜色输入绑定
    ITexture* GetCurrentColorInput() const;

    /// @brief 获取深度输入绑定
    ITexture* GetDepthInput() const;

    /// @brief 获取运动矢量输入绑定
    ITexture* GetMotionVectorInput() const;

    /// @brief 获取曝光输入绑定（DLSS 必需）
    ITexture* GetExposureInput() const;

    // === 输出资源访问 ===

    /// @brief 获取超分辨率输出
    ITexture* GetUpscaledOutput() const;

    // === 历史资源访问 ===

    /// @brief 获取历史帧颜色
    ITexture* GetHistoryColor() const;

    /// @brief 准备下一帧的资源 (交换历史缓冲区)
    void PrepareNextFrame();

    // === 资源查询 ===

    /// @brief 获取渲染宽度
    uint32_t GetRenderWidth() const { return m_renderWidth; }

    /// @brief 获取渲染高度
    uint32_t GetRenderHeight() const { return m_renderHeight; }

    /// @brief 获取显示宽度
    uint32_t GetDisplayWidth() const { return m_displayWidth; }

    /// @brief 获取显示高度
    uint32_t GetDisplayHeight() const { return m_displayHeight; }

private:
    bool CreateTextures();
    void ReleaseTextures();

    // 渲染设备
    IRenderDevice* m_device = nullptr;

    // 分辨率配置
    uint32_t m_renderWidth = 0;
    uint32_t m_renderHeight = 0;
    uint32_t m_displayWidth = 0;
    uint32_t m_displayHeight = 0;
    uint32_t m_maxFramesInFlight = 2;

    // 当前帧索引
    uint32_t m_currentFrameIndex = 0;

    // === 输入资源 ===

    // 颜色输入 (渲染分辨率)
    std::unique_ptr<ITexture> m_colorInput;

    // 深度输入 (渲染分辨率)
    std::unique_ptr<ITexture> m_depthInput;

    // 运动矢量 (渲染分辨率, RG16_FLOAT)
    std::unique_ptr<ITexture> m_motionVectors;

    // 曝光 (DLSS 必需, 1x1 R32_FLOAT)
    std::unique_ptr<ITexture> m_exposure;

    // === 内部资源 ===

    // DLSS 内部计算资源（由 Streamline 管理）

    // === 历史资源 (双缓冲) ===

    // 历史颜色 (显示分辨率)
    std::vector<std::unique_ptr<ITexture>> m_historyColor;

    // === 输出资源 ===

    // 超分辨率输出 (显示分辨率)
    std::unique_ptr<ITexture> m_upscaledOutput;

    // 状态
    bool m_initialized = false;
};

} // namespace PrismaEngine::Graphic
