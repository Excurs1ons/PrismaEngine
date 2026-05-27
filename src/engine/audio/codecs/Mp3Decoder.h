#pragma once

#include <Engine/audio/AudioTypes.h>
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>

#define DR_MP3_IMPLEMENTATION
#include <dr_mp3.h>

namespace Prisma::Audio::Codecs {

class Mp3Decoder {
public:
    static bool IsMp3Format(const uint8_t* header, size_t size) {
        if (size < 3) return false;
        // MP3 ID3 tag or sync word
        if (header[0] == 'I' && header[1] == 'D' && header[2] == '3') return true;
        // MPEG sync word: 0xFFE? or 0xFFF?
        if (header[0] == 0xFF && (header[1] & 0xE0) == 0xE0) return true;
        return false;
    }

    static bool Decode(const uint8_t* data, size_t size, AudioClip& outClip) {
        if (!data || size < 4) return false;

        drmp3_config config;
        drmp3_uint64 totalFrameCount = 0;
        drmp3_int16* pcmData = drmp3_open_memory_and_read_pcm_frames_s16(
            data, size, &config, &totalFrameCount, nullptr);

        if (!pcmData) return false;
        if (totalFrameCount == 0 || config.channels == 0) {
            drmp3_free(pcmData, nullptr);
            return false;
        }

        outClip.format.sampleRate = config.sampleRate;
        outClip.format.channels = static_cast<uint16_t>(config.channels);
        outClip.format.bitsPerSample = 16;

        size_t sampleCount = static_cast<size_t>(totalFrameCount) * config.channels;
        size_t byteCount = sampleCount * sizeof(drmp3_int16);
        outClip.data.resize(byteCount);
        memcpy(outClip.data.data(), pcmData, byteCount);

        outClip.duration = static_cast<float>(totalFrameCount) / static_cast<float>(config.sampleRate);

        drmp3_free(pcmData, nullptr);
        return true;
    }

    static bool DecodeFile(const std::string& path, AudioClip& outClip) {
        drmp3_config config;
        drmp3_uint64 totalFrameCount = 0;
        drmp3_int16* pcmData = drmp3_open_file_and_read_pcm_frames_s16(
            path.c_str(), &config, &totalFrameCount, nullptr);

        if (!pcmData) return false;
        if (totalFrameCount == 0 || config.channels == 0) {
            drmp3_free(pcmData, nullptr);
            return false;
        }

        outClip.format.sampleRate = config.sampleRate;
        outClip.format.channels = static_cast<uint16_t>(config.channels);
        outClip.format.bitsPerSample = 16;

        size_t sampleCount = static_cast<size_t>(totalFrameCount) * config.channels;
        size_t byteCount = sampleCount * sizeof(drmp3_int16);
        outClip.data.resize(byteCount);
        memcpy(outClip.data.data(), pcmData, byteCount);

        outClip.duration = static_cast<float>(totalFrameCount) / static_cast<float>(config.sampleRate);

        drmp3_free(pcmData, nullptr);
        return true;
    }
};

} // namespace Prisma::Audio::Codecs
