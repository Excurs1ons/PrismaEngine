#pragma once

#include "IAudioDevice.h"
#include "AudioTypes.h"
#include <memory>
#include <string>
#include <vector>

namespace Prisma::Audio {

class AudioDeviceSDL3;
class AudioDeviceNull;

class AudioAPI {
public:
    static std::unique_ptr<IAudioDevice> CreateDevice(AudioDeviceType deviceType,
                                                      const AudioDesc& desc = {});

    static std::unique_ptr<IAudioDevice> CreateBestDevice(const AudioDesc& desc = {});

    static std::vector<AudioDeviceType> GetSupportedDevices();

    static bool IsDeviceSupported(AudioDeviceType deviceType);

    static AudioDeviceType GetRecommendedDevice();

    static std::string GetDeviceName(AudioDeviceType deviceType);

    static std::string GetDeviceDescription(AudioDeviceType deviceType);

    static std::string GetDeviceVersion(AudioDeviceType deviceType);

    static void PrintSupportedDevices();

    static bool TestDeviceAvailability(AudioDeviceType deviceType);

private:
    static std::unique_ptr<IAudioDevice> CreateSDL3Device(const AudioDesc& desc);

    static std::unique_ptr<IAudioDevice> CreateNullDevice(const AudioDesc& desc);

    static AudioDeviceType GetDeviceFromEnvironment();

    static AudioDeviceType GetDeviceFromConfig();
};

inline std::string AudioAPI::GetDeviceName(AudioDeviceType deviceType) {
    switch (deviceType) {
        case AudioDeviceType::Auto: return "Auto";
        case AudioDeviceType::SDL3: return "SDL3 Audio";
        case AudioDeviceType::Null: return "Null (Silent)";
        default: return "Unknown";
    }
}

inline std::string AudioAPI::GetDeviceDescription(AudioDeviceType deviceType) {
    switch (deviceType) {
        case AudioDeviceType::Auto:
            return "Auto select best audio device";
        case AudioDeviceType::SDL3:
            return "Cross-platform simple audio API";
        case AudioDeviceType::Null:
            return "Silent device for testing";
        default:
            return "Unknown audio device";
    }
}

} // namespace Prisma::Audio
