#pragma once

#include "audio/AudioTypes.h"
#include <vector>
#include <string>
#include <cstdint>

// stb_vorbis is included via fetchcontent; we forward-declare the API here
// using a local implementation pattern to avoid header pollution
#define STB_VORBIS_HEADER_ONLY
#include <stb_vorbis.c>

namespace Prisma::Audio::Codecs {

class OggDecoder {
public:
    static bool IsOggFormat(const uint8_t* header, size_t size) {
        if (size < 4) return false;
        return header[0] == 'O' && header[1] == 'g' && header[2] == 'g' && header[3] == 'S';
    }

    static bool Decode(const uint8_t* data, size_t size, AudioClip& outClip) {
        if (!data || size < 4) return false;

        int channels = 0, sampleRate = 0;
        short* output = nullptr;
        int totalSamples = stb_vorbis_decode_memory(
            reinterpret_cast<const unsigned char*>(data),
            static_cast<int>(size),
            &channels, &sampleRate,
            &output
        );

        if (totalSamples <= 0) return false;

        outClip.format.sampleRate = static_cast<uint32_t>(sampleRate);
        outClip.format.channels = static_cast<uint16_t>(channels);
        outClip.format.bitsPerSample = 16;

        size_t byteCount = static_cast<size_t>(totalSamples) * channels * sizeof(short);
        outClip.data.resize(byteCount);
        memcpy(outClip.data.data(), output, byteCount);

        outClip.duration = static_cast<float>(totalSamples) / sampleRate;

        free(output);
        return true;
    }

    static bool DecodeFile(const std::string& path, AudioClip& outClip) {
        int channels = 0, sampleRate = 0;
        short* output = nullptr;
        int totalSamples = stb_vorbis_decode_filename(
            path.c_str(),
            &channels, &sampleRate,
            &output
        );

        if (totalSamples <= 0) return false;

        outClip.format.sampleRate = static_cast<uint32_t>(sampleRate);
        outClip.format.channels = static_cast<uint16_t>(channels);
        outClip.format.bitsPerSample = 16;

        size_t byteCount = static_cast<size_t>(totalSamples) * channels * sizeof(short);
        outClip.data.resize(byteCount);
        memcpy(outClip.data.data(), output, byteCount);

        outClip.duration = static_cast<float>(totalSamples) / sampleRate;

        free(output);
        return true;
    }
};

} // namespace Prisma::Audio::Codecs
