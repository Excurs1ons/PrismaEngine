#pragma once

#include "IAudioDevice.h"
#include "AudioTypes.h"
#include <memory>
#include <string>
#include <vector>

namespace Engine::Audio {

// 前置声明设备类
class AudioDeviceOpenAL;
class AudioDeviceSDL3;
class AudioDeviceXAudio2;
class AudioDeviceNull;

/// @brief 音频设备工厂
/// 负责根据平台和配置创建合适的音频设备
class AudioFactory {
public:
    // ========== 工厂方法 ==========

    /// @brief 创建音频设备
    /// @param deviceType 设备类型
    /// @param desc 设备描述
    /// @return 设备智能指针，失败返回nullptr
    static std::unique_ptr<IAudioDevice> CreateDevice(AudioDeviceType deviceType,
                                                    const AudioDesc& desc = {});

    /// @brief 自动选择并创建最佳音频设备
    /// @param desc 设备描述
    /// @return 设备智能指针，失败返回nullptr
    static std::unique_ptr<IAudioDevice> CreateBestDevice(const AudioDesc& desc = {});

    // ========== 平台检测 ==========

    /// @brief 获取当前平台支持的设备
    /// @return 支持的设备列表
    static std::vector<AudioDeviceType> GetSupportedDevices();

    /// @brief 检查设备是否受支持
    /// @param deviceType 设备类型
    /// @return 是否支持
    static bool IsDeviceSupported(AudioDeviceType deviceType);

    /// @brief 获取推荐的默认设备
    /// @return 推荐的设备类型
    static AudioDeviceType GetRecommendedDevice();

    // ========== 设备信息 ==========

    /// @brief 获取设备名称
    /// @param deviceType 设备类型
    /// @return 设备名称
    static std::string GetDeviceName(AudioDeviceType deviceType);

    /// @brief 获取设备描述
    /// @param deviceType 设备类型
    /// @return 设备描述
    static std::string GetDeviceDescription(AudioDeviceType deviceType);

    // ========== 版本信息 ==========

    /// @brief 检查设备版本
    /// @param deviceType 设备类型
    /// @return 版本号，不支持返回空字符串
    static std::string GetDeviceVersion(AudioDeviceType deviceType);

    // ========== 调试 ==========

    /// @brief 打印所有支持的设备信息
    static void PrintSupportedDevices();

    /// @brief 测试设备可用性
    /// @param deviceType 设备类型
    /// @return 测试结果
    static bool TestDeviceAvailability(AudioDeviceType deviceType);

private:
    // ========== 内部工厂方法 ==========

    /// @brief 创建OpenAL设备
    static std::unique_ptr<IAudioDevice> CreateOpenALDevice(const AudioDesc& desc);

    /// @brief 创建SDL3设备
    static std::unique_ptr<IAudioDevice> CreateSDL3Device(const AudioDesc& desc);

    /// @brief 创建XAudio2设备
    static std::unique_ptr<IAudioDevice> CreateXAudio2Device(const AudioDesc& desc);

    /// @brief 创建空设备（静音）
    static std::unique_ptr<IAudioDevice> CreateNullDevice(const AudioDesc& desc);

    // ========== 平台相关辅助 ==========

    /// @brief 检测当前平台
    enum class Platform {
        Windows,
        Linux,
        macOS,
        Android,
        iOS,
        Unknown
    };

    static Platform GetCurrentPlatform();

    /// @brief 检查环境变量
    static AudioDeviceType GetDeviceFromEnvironment();

    /// @brief 检查配置文件
    static AudioDeviceType GetDeviceFromConfig();
};

// ========== 内联实现 ==========

inline std::string AudioFactory::GetDeviceName(AudioDeviceType deviceType) {
    switch (deviceType) {
        case AudioDeviceType::Auto: return "Auto";
        case AudioDeviceType::OpenAL: return "OpenAL";
        case AudioDeviceType::XAudio2: return "XAudio2";
        case AudioDeviceType::SDL3: return "SDL3 Audio";
        case AudioDeviceType::Null: return "Null (Silent)";
        default: return "Unknown";
    }
}

inline std::string AudioFactory::GetDeviceDescription(AudioDeviceType deviceType) {
    switch (deviceType) {
        case AudioDeviceType::Auto:
            return "自动选择最佳音频设备";
        case AudioDeviceType::OpenAL:
            return "跨平台3D音频API，支持专业音频功能";
        case AudioDeviceType::XAudio2:
            return "Windows高性能音频API，低延迟";
        case AudioDeviceType::SDL3:
            return "跨平台简单音频API，易于使用";
        case AudioDeviceType::Null:
            return "静音设备，用于测试";
        default:
            return "未知音频设备";
    }
}

} // namespace Engine::Audio