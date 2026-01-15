#pragma once

#include "interfaces/ITexture.h"
#include "interfaces/IBuffer.h"
#include <memory>
#include <vector>

namespace PrismaEngine::Graphic {

// 前置声明
class IRenderDevice;

/// @brief FSR 3.1 GPU 资源管理器
/// 管理 FSR 超分辨率所需的所有 GPU 资源
class FSRResources {
public:
    FSRResources();
    ~FSRResources();

    /// @brief 初始化 FSR 资源
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

    /// @brief 获取曝光输入绑定 (可选)
    ITexture* GetExposureInput() const;

    // === 输出资源访问 ===

    /// @brief 获取超分辨率输出
    ITexture* GetUpscaledOutput() const;

    /// @brief 获取 RCAS 输出 (锐化后)
    ITexture* GetRCASOutput() const;

    // === 历史资源访问 ===

    /// @brief 获取历史帧颜色
    ITexture* GetHistoryColor() const;

    /// @brief 获取历史帧深度
    ITexture* GetHistoryDepth() const;

    /// @brief 准备下一帧的资源 (交换历史缓冲区)
    void PrepareNextFrame();

    // === 常量缓冲区 ===

    /// @brief 获取 FSR 常量缓冲区
    IBuffer* GetConstantBuffer() const;

    /// @brief 更新常量缓冲区数据
    /// @param data 数据指针
    /// @param size 数据大小
    void UpdateConstantBuffer(const void* data, uint32_t size);

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
    bool CreateBuffers();
    void ReleaseTextures();
    void ReleaseBuffers();

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

    // 曝光 (可选, 1x1 R32_FLOAT)
    std::unique_ptr<ITexture> m_exposure;

    // === 内部资源 ===

    // 自动曝光计算 (内部使用)
    std::unique_ptr<ITexture> m_autoExposure;

    // 锁定掩码 (用于重锁定)
    std::unique_ptr<ITexture> m_lockMask;

    // 新锁定掩码
    std::unique_ptr<ITexture> m_lockNewMask;

    // === 历史资源 (双缓冲) ===

    // 历史颜色 (显示分辨率)
    std::vector<std::unique_ptr<ITexture>> m_historyColor;

    // 历史深度 (显示分辨率)
    std::vector<std::unique_ptr<ITexture>> m_historyDepth;

    // === 输出资源 ===

    // 超分辨率输出 (显示分辨率, EASU 之后)
    std::unique_ptr<ITexture> m_upscaledOutput;

    // RCAS 输出 (显示分辨率, 锐化之后)
    std::unique_ptr<ITexture> m_rcasOutput;

    // === 常量缓冲区 ===

    // FSR 常量缓冲区
    std::unique_ptr<IBuffer> m_constantBuffer;

    // 状态
    bool m_initialized = false;
};

} // namespace PrismaEngine::Graphic
