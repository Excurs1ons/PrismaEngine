#include "AudioAPI.h"

// 音频设备头文件 - 仅包含通用接口，具体实现由 CMake 控制
#include "AudioDeviceNull.h"  // Null 设备总是可用

#ifdef PRISMA_ENABLE_AUDIO_XAUDIO2
#include "AudioDeviceXAudio2.h"
#endif

#ifdef PRISMA_ENABLE_AUDIO_OPENAL
#include "AudioDeviceOpenAL.h"
#endif

#ifdef PRISMA_ENABLE_AUDIO_SDL3
#include <SDL3/SDL.h>
#include "AudioDeviceSDL3.h"
#endif
#include "../Logger.h"
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iterator>

namespace PrismaEngine::Audio {

// ========== 工厂方法实现 ==========

std::unique_ptr<IAudioDevice> AudioAPI::CreateDevice(AudioDeviceType deviceType,
                                                        const AudioDesc& desc) {
    LOG_INFO("Audio", "创建音频设备，设备类型: {} ({})",
             GetDeviceName(deviceType),
             static_cast<int>(deviceType));

    // 检查设备是否支持
    if (!IsDeviceSupported(deviceType)) {
        LOG_ERROR("Audio", "不支持的音频设备: {} ({})",
                  GetDeviceName(deviceType),
                  static_cast<int>(deviceType));

        // 尝试使用默认设备
        LOG_INFO("Audio", "尝试使用默认设备...");
        return CreateBestDevice(desc);
    }

    switch (deviceType) {
#if defined(PRISMA_ENABLE_AUDIO_OPENAL)
        case AudioDeviceType::OpenAL:
            return CreateOpenALDevice(desc);
#endif

#if defined(PRISMA_ENABLE_AUDIO_SDL3)
        case AudioDeviceType::SDL3:
            return CreateSDL3Device(desc);
#endif

#if defined(PRISMA_ENABLE_AUDIO_XAUDIO2)
        case AudioDeviceType::XAudio2:
            return CreateXAudio2Device(desc);
#endif

        case AudioDeviceType::Null:
            return CreateNullDevice(desc);

        case AudioDeviceType::Auto:
        default:
            return CreateBestDevice(desc);
    }
}

std::unique_ptr<IAudioDevice> AudioAPI::CreateBestDevice(const AudioDesc& desc) {
    // 1. 首先检查环境变量
    AudioDeviceType envDevice = GetDeviceFromEnvironment();
    if (envDevice != AudioDeviceType::Auto && IsDeviceSupported(envDevice)) {
        LOG_INFO("Audio", "使用环境变量指定的音频设备: {}", GetDeviceName(envDevice));
        return CreateDevice(envDevice, desc);
    }

    // 2. 检查配置文件
    AudioDeviceType configDevice = GetDeviceFromConfig();
    if (configDevice != AudioDeviceType::Auto && IsDeviceSupported(configDevice)) {
        LOG_INFO("Audio", "使用配置文件指定的音频设备: {}", GetDeviceName(configDevice));
        return CreateDevice(configDevice, desc);
    }

    // 3. 使用平台推荐的默认设备
    AudioDeviceType recommended = GetRecommendedDevice();
    LOG_INFO("Audio", "使用平台推荐的音频设备: {}", GetDeviceName(recommended));

    auto device = CreateDevice(recommended, desc);
    if (device) {
        return device;
    }

    // 4. 如果都失败了，使用静音设备
    LOG_WARNING("Audio", "所有音频设备初始化失败，使用静音设备");
    return CreateNullDevice(desc);
}

// ========== 平台检测 ==========

std::vector<AudioDeviceType> AudioAPI::GetSupportedDevices() {
    std::vector<AudioDeviceType> devices;

    Platform platform = GetCurrentPlatform();

    // 所有平台都支持Null设备
    devices.push_back(AudioDeviceType::Null);

    // 根据平台添加支持的设备
    switch (platform) {
        case Platform::Windows:
#if defined(PRISMA_ENABLE_AUDIO_XAUDIO2)
            devices.push_back(AudioDeviceType::XAudio2);
#endif
#if defined(PRISMA_ENABLE_AUDIO_OPENAL)
            devices.push_back(AudioDeviceType::OpenAL);
#endif
#if defined(PRISMA_ENABLE_AUDIO_SDL3)
            devices.push_back(AudioDeviceType::SDL3);
#endif
            break;

        case Platform::Linux:
        case Platform::Android:
#if defined(PRISMA_ENABLE_AUDIO_OPENAL)
            devices.push_back(AudioDeviceType::OpenAL);
#endif
#if defined(PRISMA_ENABLE_AUDIO_SDL3)
            devices.push_back(AudioDeviceType::SDL3);
#endif
            devices.push_back(AudioDeviceType::SDL3);
            break;

        case Platform::macOS:
        case Platform::iOS:
            devices.push_back(AudioDeviceType::OpenAL);
            devices.push_back(AudioDeviceType::SDL3);
            break;

        default:
            LOG_WARNING("Audio", "未知平台，仅支持Null设备");
            break;
    }

    return devices;
}

bool AudioAPI::IsDeviceSupported(AudioDeviceType deviceType) {
    // Auto和Null总是支持的
    if (deviceType == AudioDeviceType::Auto || deviceType == AudioDeviceType::Null) {
        return true;
    }

    auto supported = GetSupportedDevices();
    return std::find(supported.begin(), supported.end(), deviceType) != supported.end();
}

AudioDeviceType AudioAPI::GetRecommendedDevice() {
    Platform platform = GetCurrentPlatform();

    switch (platform) {
        case Platform::Windows:
#if defined(PRISMA_ENABLE_AUDIO_XAUDIO2)
            // Windows首选XAudio2，性能最好
            return AudioDeviceType::XAudio2;
#elif defined(PRISMA_ENABLE_AUDIO_SDL3)
            return AudioDeviceType::SDL3;
#else
            return AudioDeviceType::Null;
#endif

        case Platform::Linux:
        case Platform::Android:
#if defined(PRISMA_ENABLE_AUDIO_OPENAL)
            // Linux/Android首选OpenAL，功能最全
            return AudioDeviceType::OpenAL;
#elif defined(PRISMA_ENABLE_AUDIO_SDL3)
            return AudioDeviceType::SDL3;
#else
            return AudioDeviceType::Null;
#endif

        case Platform::macOS:
        case Platform::iOS:
#if defined(PRISMA_ENABLE_AUDIO_OPENAL)
            // macOS/iOS使用OpenAL（Core Audio通过OpenAL暴露）
            return AudioDeviceType::OpenAL;
#elif defined(PRISMA_ENABLE_AUDIO_SDL3)
            return AudioDeviceType::SDL3;
#else
            return AudioDeviceType::Null;
#endif

        default:
#if defined(PRISMA_ENABLE_AUDIO_SDL3)
            return AudioDeviceType::SDL3; // SDL3作为通用备选
#else
            return AudioDeviceType::Null;
#endif
    }
}

// ========== 版本信息 ==========

std::string AudioAPI::GetDeviceVersion(AudioDeviceType deviceType) {
    switch (deviceType) {
#if defined(PRISMA_ENABLE_AUDIO_OPENAL)
        case AudioDeviceType::OpenAL:
            // 查询OpenAL版本
            if (IsDeviceSupported(AudioDeviceType::OpenAL)) {
                auto device = CreateOpenALDevice({});
                if (device) {
                    auto info = device->GetDeviceInfo();
                    return info.version;
                }
            }
            break;
#endif

#if defined(PRISMA_ENABLE_AUDIO_SDL3)
        case AudioDeviceType::SDL3:
            // 查询SDL3版本
            if (IsDeviceSupported(AudioDeviceType::SDL3)) {
                auto device = CreateSDL3Device({});
                if (device) {
                    auto info = device->GetDeviceInfo();
                    return info.version;
                }
            }
            break;
#endif

#if defined(PRISMA_ENABLE_AUDIO_XAUDIO2)
        case AudioDeviceType::XAudio2:
            // XAudio2 版本信息
            if (IsDeviceSupported(AudioDeviceType::XAudio2)) {
                auto device = CreateXAudio2Device({});
                if (device) {
                    auto info = device->GetDeviceInfo();
                    return info.version;
                }
            }
            break;
#endif

        case AudioDeviceType::Null:
            return "1.0 (Null)";

        default:
            return "Unknown";
    }

    return "Unknown";
}

// ========== 调试 ==========

void AudioAPI::PrintSupportedDevices() {
    LOG_INFO("Audio", "=== 支持的音频设备 ===");

    auto devices = GetSupportedDevices();
    AudioDeviceType recommended = GetRecommendedDevice();

    for (auto device : devices) {
        std::string version = GetDeviceVersion(device);
        std::string mark = (device == recommended) ? " [推荐]" : "";

        LOG_INFO("Audio", "  {}{} - {}{}",
                 GetDeviceName(device),
                 mark,
                 GetDeviceDescription(device),
                 version.empty() ? "" : (" (v" + version + ")"));
    }

    LOG_INFO("Audio", "====================");
}

bool AudioAPI::TestDeviceAvailability(AudioDeviceType deviceType) {
    if (!IsDeviceSupported(deviceType)) {
        return false;
    }

    AudioDesc testDesc;
    testDesc.maxVoices = 1;
    testDesc.bufferSize = 256;

    auto device = CreateDevice(deviceType, testDesc);
    if (!device) {
        return false;
    }

    bool success = device->IsInitialized();
    device->Shutdown();

    return success;
}

// ========== 私有方法实现 ==========

#ifdef PRISMA_ENABLE_AUDIO_OPENAL
std::unique_ptr<IAudioDevice> AudioAPI::CreateOpenALDevice(const AudioDesc& desc) {
    auto device = std::make_unique<AudioDeviceOpenAL>();
    if (device->Initialize(desc)) {
        return device;
    }
    return nullptr;
}
#endif

#ifdef PRISMA_ENABLE_AUDIO_SDL3
std::unique_ptr<IAudioDevice> AudioAPI::CreateSDL3Device(const AudioDesc& desc) {
    auto device = std::make_unique<AudioDeviceSDL3>();
    if (device->Initialize(desc)) {
        return device;
    }
    return nullptr;
}
#endif

#ifdef PRISMA_ENABLE_AUDIO_XAUDIO2
std::unique_ptr<IAudioDevice> AudioAPI::CreateXAudio2Device(const AudioDesc& desc) {
    auto device = std::make_unique<AudioDeviceXAudio2>();
    if (device->Initialize(desc)) {
        return device;
    }
    return nullptr;
}
#endif

std::unique_ptr<IAudioDevice> AudioAPI::CreateNullDevice(const AudioDesc& desc) {
    auto device = std::make_unique<AudioDeviceNull>();
    if (device->Initialize(desc)) {
        return device;
    }
    return nullptr;
}

AudioAPI::Platform AudioAPI::GetCurrentPlatform() {
#if defined(_WIN32) || defined(_WIN64)
    return Platform::Windows;
#elif defined(__linux__)
    return Platform::Linux;
#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
        return Platform::iOS;
    #else
        return Platform::macOS;
    #endif
#elif defined(__ANDROID__)
    return Platform::Android;
#else
    return Platform::Unknown;
#endif
}

AudioDeviceType AudioAPI::GetDeviceFromEnvironment() {
    const char* envVar = std::getenv("PRISMA_AUDIO_DEVICE");
    if (!envVar) {
        return AudioDeviceType::Auto;
    }

    std::string device(envVar);
    std::transform(device.begin(), device.end(), device.begin(), ::tolower);

    if (device == "openal") return AudioDeviceType::OpenAL;
    if (device == "xaudio2") return AudioDeviceType::XAudio2;
    if (device == "sdl3" || device == "sdl") return AudioDeviceType::SDL3;
    if (device == "null" || device == "none") return AudioDeviceType::Null;

    LOG_WARNING("Audio", "未知的音频设备环境变量: {}", envVar);
    return AudioDeviceType::Auto;
}

AudioDeviceType AudioAPI::GetDeviceFromConfig() {
    // 这里可以读取配置文件
    // 例如：config/audio.json

    std::ifstream configFile("config/audio.json");
    if (!configFile.is_open()) {
        return AudioDeviceType::Auto;
    }

    try {
        // 简单的JSON解析（实际应使用JSON库）
        std::string content((std::istreambuf_iterator<char>(configFile)),
                           std::istreambuf_iterator<char>());

        // 查找 device 字段
        size_t devicePos = content.find("\"device\"");
        if (devicePos != std::string::npos) {
            size_t colonPos = content.find(":", devicePos);
            if (colonPos != std::string::npos) {
                size_t start = content.find("\"", colonPos);
                if (start != std::string::npos) {
                    size_t end = content.find("\"", start + 1);
                    if (end != std::string::npos) {
                        std::string device = content.substr(start + 1, end - start - 1);

                        if (device == "OpenAL") return AudioDeviceType::OpenAL;
                        if (device == "XAudio2") return AudioDeviceType::XAudio2;
                        if (device == "SDL3") return AudioDeviceType::SDL3;
                        if (device == "Null") return AudioDeviceType::Null;
                    }
                }
            }
        }
    }
    catch (const std::exception& e) {
        LOG_ERROR("Audio", "读取音频配置文件失败: {}", e.what());
    }

    return AudioDeviceType::Auto;
}

} // namespace Engine::Audio