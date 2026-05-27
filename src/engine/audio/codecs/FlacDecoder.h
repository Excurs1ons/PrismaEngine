#pragma once

#include <Engine/audio/AudioTypes.h>
#include <vector>
#include <string>
#include <cstdint>

#define DR_FLAC_IMPLEMENTATION
#include <dr_flac.h>

namespace Prisma::Audio::Codecs {

class FlacDecoder {
public:
    static bool IsFlacFormat(const uint8_t* header, size_t size) {
        if (size < 4) return false;
        return header[0] == 'f' && header[1] == 'L' && header[2] == 'a' && header[3] == 'C';
    }

    static bool Decode(const uint8_t* data, size_t size, AudioClip& outClip) {
        if (!data || size < 4) return false;

        unsigned int channels = 0, sampleRate = 0;
        drflac_uint64 totalPcmFrameCount = 0;
        drflac_int16* pcmData = drflac_open_memory_and_read_pcm_frames_s16(
            data, size, &channels, &sampleRate, &totalPcmFrameCount, nullptr
        );

        if (!pcmData) return false;

        outClip.format.sampleRate = sampleRate;
        outClip.format.channels = static_cast<uint16_t>(channels);
        outClip.format.bitsPerSample = 16;

        size_t sampleCount = static_cast<size_t>(totalPcmFrameCount) * channels;
        size_t byteCount = sampleCount * sizeof(drflac_int16);
        outClip.data.resize(byteCount);
        memcpy(outClip.data.data(), pcmData, byteCount);

        outClip.duration = static_cast<float>(totalPcmFrameCount) / sampleRate;

        drflac_free(pcmData, nullptr);
        return true;
    }

    static bool DecodeFile(const std::string& path, AudioClip& outClip) {
        unsigned int channels = 0, sampleRate = 0;
        drflac_uint64 totalPcmFrameCount = 0;
        drflac_int16* pcmData = drflac_open_file_and_read_pcm_frames_s16(
            path.c_str(), &channels, &sampleRate, &totalPcmFrameCount, nullptr
        );

        if (!pcmData) return false;

        outClip.format.sampleRate = sampleRate;
        outClip.format.channels = static_cast<uint16_t>(channels);
        outClip.format.bitsPerSample = 16;

        size_t sampleCount = static_cast<size_t>(totalPcmFrameCount) * channels;
        size_t byteCount = sampleCount * sizeof(drflac_int16);
        outClip.data.resize(byteCount);
        memcpy(outClip.data.data(), pcmData, byteCount);

        outClip.duration = static_cast<float>(totalPcmFrameCount) / sampleRate;

        drflac_free(pcmData, nullptr);
        return true;
    }
};

} // namespace Prisma::Audio::Codecs
