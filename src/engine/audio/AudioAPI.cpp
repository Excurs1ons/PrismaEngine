#include "AudioAPI.h"
#include "AudioDeviceNull.h"
#include "AudioDeviceSDL3.h"
#include "backends/AudioDeviceMiniaudio.h"
#include "codecs/OggDecoder.h"
#include "codecs/Mp3Decoder.h"
#include "codecs/FlacDecoder.h"
#include <SDL3/SDL.h>

#include "../logger/Logger.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>

namespace Prisma::Audio {

std::unique_ptr<IAudioDevice> AudioAPI::CreateDevice(AudioDeviceType deviceType, const AudioDesc& desc) {
    if (!IsDeviceSupported(deviceType)) {
        return CreateBestDevice(desc);
    }

    switch (deviceType) {
        case AudioDeviceType::SDL3:
            return CreateSDL3Device(desc);

        case AudioDeviceType::Miniaudio:
            return CreateMiniaudioDevice(desc);

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

std::shared_ptr<AudioClip> AudioAPI::LoadClip(const std::string& path) {
    std::string ext = std::filesystem::path(path).extension().string();
    for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    auto clip = std::make_shared<AudioClip>();

#ifdef PRISMA_ENABLE_AUDIO_CODEC_OGG
    if (ext == ".ogg") {
        if (Codecs::OggDecoder::DecodeFile(path, *clip)) {
            clip->path = path;
            LOG_DEBUG("Audio", "成功加载 Ogg Vorbis: {0} ({1}s)", path, clip->duration);
            return clip;
        }
        LOG_ERROR("Audio", "加载 Ogg 失败: {0}", path);
        return nullptr;
    }
#endif

#ifdef PRISMA_ENABLE_AUDIO_CODEC_MP3
    if (ext == ".mp3") {
        if (Codecs::Mp3Decoder::DecodeFile(path, *clip)) {
            clip->path = path;
            LOG_DEBUG("Audio", "成功加载 MP3: {0} ({1}s)", path, clip->duration);
            return clip;
        }
        LOG_ERROR("Audio", "加载 MP3 失败: {0}", path);
        return nullptr;
    }
#endif

#ifdef PRISMA_ENABLE_AUDIO_CODEC_FLAC
    if (ext == ".flac") {
        if (Codecs::FlacDecoder::DecodeFile(path, *clip)) {
            clip->path = path;
            LOG_DEBUG("Audio", "成功加载 FLAC: {0} ({1}s)", path, clip->duration);
            return clip;
        }
        LOG_ERROR("Audio", "加载 FLAC 失败: {0}", path);
        return nullptr;
    }
#endif

    if (ext == ".wav") {
        return LoadWAV(path);
    }

    LOG_ERROR("Audio", "不支持的音频格式: {0}", path);
    return nullptr;
}

std::shared_ptr<AudioClip> AudioAPI::LoadWAV(const std::string& path) {
    SDL_AudioSpec spec;
    Uint8* data = nullptr;
    Uint32 len = 0;

    if (!SDL_LoadWAV(path.c_str(), &spec, &data, &len)) {
        LOG_ERROR("Audio", "加载 WAV 失败: {0}, Error: {1}", path, SDL_GetError());
        return nullptr;
    }

    auto clip = std::make_shared<AudioClip>();
    clip->path = path;
    clip->format.sampleRate = static_cast<uint32_t>(spec.freq);
    clip->format.channels = static_cast<uint16_t>(spec.channels);
    
    // SDL3 spec.format 转换
    if (spec.format == SDL_AUDIO_F32) {
        clip->format.bitsPerSample = 32;
    } else if (spec.format == SDL_AUDIO_S16) {
        clip->format.bitsPerSample = 16;
    } else {
        clip->format.bitsPerSample = 16;
    }

    clip->data.assign(data, data + len);
    
    uint32_t bytesPerSecond = clip->format.sampleRate * clip->format.channels * (clip->format.bitsPerSample / 8);
    clip->duration = static_cast<float>(len) / static_cast<float>(std::max<uint32_t>(1, bytesPerSecond));

    SDL_free(data);
    LOG_DEBUG("Audio", "成功加载音频: {0} ({1}s)", path, clip->duration);
    return clip;
}

std::vector<AudioDeviceType> AudioAPI::GetSupportedDevices() {
    std::vector<AudioDeviceType> devices;
    devices.push_back(AudioDeviceType::Null);
#ifdef PRISMA_ENABLE_AUDIO_SDL3
    devices.push_back(AudioDeviceType::SDL3);
#endif
#ifdef PRISMA_ENABLE_AUDIO_MINIAUDIO
    devices.push_back(AudioDeviceType::Miniaudio);
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

    if (deviceType == AudioDeviceType::Miniaudio) {
#ifdef PRISMA_ENABLE_AUDIO_MINIAUDIO
        return true;
#endif
    }

    return false;
}

AudioDeviceType AudioAPI::GetRecommendedDevice() {
#ifdef PRISMA_ENABLE_AUDIO_SDL3
    return AudioDeviceType::SDL3;
#elif defined(PRISMA_ENABLE_AUDIO_MINIAUDIO)
    return AudioDeviceType::Miniaudio;
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

    if (deviceType == AudioDeviceType::Miniaudio) {
#ifdef PRISMA_ENABLE_AUDIO_MINIAUDIO
        auto device = CreateMiniaudioDevice({});
        if (device) {
            auto info = device->GetDeviceInfo();
            return info.version;
        }
#endif
        return "0.11.21 (Miniaudio)";
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
    testDesc.maxVoices  = 1;
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

#ifdef PRISMA_ENABLE_AUDIO_MINIAUDIO
std::unique_ptr<IAudioDevice> AudioAPI::CreateMiniaudioDevice(const AudioDesc& desc) {
    auto device = std::make_unique<AudioDeviceMiniaudio>();
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

    if (device == "sdl3" || device == "sdl")
        return AudioDeviceType::SDL3;
    if (device == "miniaudio" || device == "mini")
        return AudioDeviceType::Miniaudio;
    if (device == "null" || device == "none")
        return AudioDeviceType::Null;

    return AudioDeviceType::Auto;
}

AudioDeviceType AudioAPI::GetDeviceFromConfig() {
    std::ifstream configFile("config/audio.json");
    if (!configFile.is_open()) {
        return AudioDeviceType::Auto;
    }

    return AudioDeviceType::Auto;
}

}  // namespace Prisma::Audio
