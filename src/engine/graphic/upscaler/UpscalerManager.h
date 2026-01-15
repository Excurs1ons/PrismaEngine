#pragma once

#include "interfaces/IUpscaler.h"
#include <memory>
#include <vector>
#include <unordered_map>

namespace PrismaEngine::Graphic {

// 前置声明
class IRenderDevice;

/// @brief 超分辨率管理器 / Upscaler Manager
/// 负责管理多个超分辨率器实例，提供统一的访问接口
/// Manages multiple upscaler instances and provides unified access
class UpscalerManager {
public:
    /// @brief 获取单例实例 / Get singleton instance
    static UpscalerManager& Instance();

    /// @brief 初始化管理器（自动创建可用的超分辨率器）/ Initialize manager (auto-create available upscalers)
    /// @param device 渲染设备 / Render device
    void Initialize(IRenderDevice* device);

    /// @brief 关闭管理器 / Shutdown manager
    void Shutdown();

    /// @brief 获取可用的超分辨率技术列表 / Get list of available upscaler technologies
    /// @return 技术类型列表 / List of technology types
    std::vector<UpscalerTechnology> GetAvailableTechnologies() const;

    /// @brief 创建超分辨率器 / Create upscaler
    /// @param technology 技术类型 / Technology type
    /// @param desc 初始化描述 / Initialization description
    /// @return 超分辨率器接口指针，失败返回 nullptr / Upscaler interface pointer, nullptr on failure
    IUpscaler* CreateUpscaler(UpscalerTechnology technology,
                              const UpscalerInitDesc& desc);

    /// @brief 获取当前活动的超分辨率器 / Get active upscaler
    IUpscaler* GetActiveUpscaler() const { return m_activeUpscaler; }

    /// @brief 设置当前活动的超分辨率器 / Set active upscaler
    /// @param upscaler 超分辨率器指针 / Upscaler pointer
    void SetActiveUpscaler(IUpscaler* upscaler);

    /// @brief 根据技术类型获取超分辨率器 / Get upscaler by technology type
    IUpscaler* GetUpscaler(UpscalerTechnology technology) const;

    /// @brief 获取默认超分辨率技术 / Get default upscaler technology
    /// @return 默认技术类型（平台相关）/ Default technology type (platform-dependent)
    static UpscalerTechnology GetDefaultTechnology();

    /// @brief 获取技术信息 / Get technology information
    /// @param technology 技术类型 / Technology type
    /// @return 技术信息，不支持返回空 / Technology info, empty if not supported
    UpscalerInfo GetTechnologyInfo(UpscalerTechnology technology) const;

    /// @brief 检查技术是否可用 / Check if technology is available
    bool IsTechnologyAvailable(UpscalerTechnology technology) const;

    /// @brief 检查是否已初始化 / Check if initialized
    bool IsInitialized() const { return m_initialized; }

private:
    UpscalerManager() = default;
    ~UpscalerManager() = default;
    UpscalerManager(const UpscalerManager&) = delete;
    UpscalerManager& operator=(const UpscalerManager&) = delete;

    void CreateAvailableUpscalers(IRenderDevice* device);

    std::unordered_map<UpscalerTechnology, std::unique_ptr<IUpscaler>> m_upscalers;
    IUpscaler* m_activeUpscaler = nullptr;
    bool m_initialized = false;
};

/// @brief 全局辅助函数 / Global helper functions
namespace UpscalerHelper {
    /// @brief 获取技术名称 / Get technology name
    std::string GetTechnologyName(UpscalerTechnology technology);

    /// @brief 获取质量模式名称 / Get quality mode name
    std::string GetQualityName(UpscalerQuality quality);

    /// @brief 计算渲染分辨率 / Calculate render resolution
    void CalculateRenderResolution(UpscalerQuality quality,
                                    uint32_t displayWidth,
                                    uint32_t displayHeight,
                                    uint32_t& outWidth,
                                    uint32_t& outHeight);

    /// @brief 生成抖动序列（Halton 序列）/ Generate jitter sequence (Halton sequence)
    /// @param index 索引 / Index
    /// @param x 输出 X / Output X
    /// @param y 输出 Y / Output Y
    void GenerateHaltonSequence(int index, float& x, float& y);

    /// @brief 获取质量模式的缩放因子 / Get scale factor for quality mode
    /// @param quality 质量模式 / Quality mode
    /// @return 缩放因子 / Scale factor (e.g., 1.5 for Quality mode)
    float GetScaleFactor(UpscalerQuality quality);
}

} // namespace PrismaEngine::Graphic
