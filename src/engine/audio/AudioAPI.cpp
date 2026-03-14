#include "AudioAPI.h"
#include "AudioDeviceNull.h"
#include <SDL3/SDL.h>
#include "AudioDeviceSDL3.h"

#include "../Logger.h"
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iterator>

namespace Prisma::Audio {

std::unique_ptr<IAudioDevice> AudioAPI::CreateDevice(AudioDeviceType deviceType,
                                                       const AudioDesc& desc) {
    if (!IsDeviceSupported(deviceType)) {
        return CreateBestDevice(desc);
    }

    switch (deviceType) {
        case AudioDeviceType::SDL3:
            return CreateSDL3Device(desc);

        case AudioDeviceType::Null:
            return CreateNullDevice(desc);

        case AudioDeviceType::Auto:
        default:
            return CreateBestDevice(desc);
    }
}

std::unique_ptr<IAudioDevice> AudioAPI::CreateBestDevice(const AudioDesc& desc) {
    AudioDeviceType envDevice = GetDeviceFromEnvironment();
    if (envDevice != AudioDeviceType::Auto && IsDeviceSupported(envDevice)) {
        return CreateDevice(envDevice, desc);
    }

    AudioDeviceType configDevice = GetDeviceFromConfig();
    if (configDevice != AudioDeviceType::Auto && IsDeviceSupported(configDevice)) {
        return CreateDevice(configDevice, desc);
    }

    return CreateSDL3Device(desc);
}

std::vector<AudioDeviceType> AudioAPI::GetSupportedDevices() {
    std::vector<AudioDeviceType> devices;
    devices.push_back(AudioDeviceType::Null);
#ifdef PRISMA_ENABLE_AUDIO_SDL3
    devices.push_back(AudioDeviceType::SDL3);
#endif
    return devices;
}

bool AudioAPI::IsDeviceSupported(AudioDeviceType deviceType) {
    if (deviceType == AudioDeviceType::Auto || deviceType == AudioDeviceType::Null) {
        return true;
    }

    if (deviceType == AudioDeviceType::SDL3) {
#ifdef PRISMA_ENABLE_AUDIO_SDL3
        return true;
#endif
    }

    return false;
}

AudioDeviceType AudioAPI::GetRecommendedDevice() {
#ifdef PRISMA_ENABLE_AUDIO_SDL3
    return AudioDeviceType::SDL3;
#else
    return AudioDeviceType::Null;
#endif
}

std::string AudioAPI::GetDeviceVersion(AudioDeviceType deviceType) {
    if (deviceType == AudioDeviceType::SDL3) {
#ifdef PRISMA_ENABLE_AUDIO_SDL3
        auto device = CreateSDL3Device({});
        if (device) {
            auto info = device->GetDeviceInfo();
            return info.version;
        }
#endif
    }

    if (deviceType == AudioDeviceType::Null) {
        return "1.0 (Null)";
    }

    return "Unknown";
}

void AudioAPI::PrintSupportedDevices() {
    auto devices = GetSupportedDevices();
    for (auto device : devices) {
        std::string version = GetDeviceVersion(device);
    }
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

#ifdef PRISMA_ENABLE_AUDIO_SDL3
std::unique_ptr<IAudioDevice> AudioAPI::CreateSDL3Device(const AudioDesc& desc) {
    auto device = std::make_unique<AudioDeviceSDL3>();
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

AudioDeviceType AudioAPI::GetDeviceFromEnvironment() {
    const char* envVar = std::getenv("PRISMA_AUDIO_DEVICE");
    if (!envVar) {
        return AudioDeviceType::Auto;
    }

    std::string device(envVar);
    std::transform(device.begin(), device.end(), device.begin(), ::tolower);

    if (device == "sdl3" || device == "sdl") return AudioDeviceType::SDL3;
    if (device == "null" || device == "none") return AudioDeviceType::Null;

    return AudioDeviceType::Auto;
}

AudioDeviceType AudioAPI::GetDeviceFromConfig() {
    std::ifstream configFile("config/audio.json");
    if (!configFile.is_open()) {
        return AudioDeviceType::Auto;
    }

    return AudioDeviceType::Auto;
}

} // namespace Prisma::Audio
