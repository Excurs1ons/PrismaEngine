#pragma once

#include "Export.h"
#include "interfaces/RenderTypes.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Prisma::Graphic {

// 前置声明
class IRenderDevice;
class IDeviceContext;

// Headless 渲染器配置
/// 用于初始化离屏渲染设备
struct HeadlessRendererConfig {
    uint32_t width              = 1920;
    uint32_t height             = 1080;
    bool enableValidation       = false;
    std::string preferredDevice;   ///< 首选 GPU 名称（空串 = 自动选择）
    PresentMode presentMode     = PresentMode::VSync;
};

// 离屏像素回读结果
struct HeadlessReadbackResult {
    std::vector<uint8_t> pixels;    ///< RGBA 像素数据
    uint32_t width  = 0;
    uint32_t height = 0;
    uint32_t channels = 4;           ///< 每像素通道数（始终为 4）
};

// Headless 渲染器
/// 离屏/服务器/自动化渲染场景，无需窗口或显示设备。
/// 内部创建并管理一个 Vulkan 渲染设备（headless 模式），
/// 提供 Init → RenderFrame → ReadbackPixels 的完整工作流。
class ENGINE_API HeadlessRenderer {
public:
    HeadlessRenderer();
    ~HeadlessRenderer();

    // 禁用拷贝
    HeadlessRenderer(const HeadlessRenderer&) = delete;
    HeadlessRenderer& operator=(const HeadlessRenderer&) = delete;

    // 初始化离屏渲染设备
    /// @param config 渲染配置（宽度、高度、验证层等）
    /// @return true 初始化成功
    bool Init(const HeadlessRendererConfig& config);

    // 关闭渲染器，释放所有资源
    void Shutdown();

    // 渲染一帧到离屏目标
    /// 内部调用 BeginFrame → (可选 RenderPass) → EndFrame → Present
    void RenderFrame();

    // 从离屏帧缓冲读取像素数据
    /// @param x, y  读取区域左上角（像素坐标）
    /// @param w, h  读取区域宽高
    /// @return 包含 RGBA 像素数据的 HeadlessReadbackResult
    HeadlessReadbackResult ReadbackPixels(uint32_t x, uint32_t y,
                                          uint32_t w, uint32_t h);

    // 获取底层渲染设备指针（用于自定义管线配置）
    IRenderDevice* GetDevice() const { return m_device.get(); }

    // 是否已成功初始化
    bool IsInitialized() const { return m_initialized; }

    // 获取当前配置
    const HeadlessRendererConfig& GetConfig() const { return m_config; }

private:
    std::unique_ptr<IRenderDevice> m_device;
    HeadlessRendererConfig m_config;
    bool m_initialized = false;
};

} // namespace Prisma::Graphic
