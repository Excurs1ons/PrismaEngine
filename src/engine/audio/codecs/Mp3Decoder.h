#pragma once

#include <Engine/audio/AudioTypes.h>
#include <vector>
#include <string>
#include <cstdint>

#define MINIMP3_IMPLEMENTATION
#define MINIMP3_ONLY_MP3
#include <minimp3.h>
#include <minimp3_ex.h>

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

        mp3dec_t dec;
        mp3dec_init(&dec);

        mp3dec_file_info_t info = {};
        if (mp3dec_load_buf(&dec, data, static_cast<int>(size), &info, nullptr, nullptr)) {
            return false;
        }

        if (info.samples == 0 || info.channels == 0) {
            free(info.buffer);
            return false;
        }

        outClip.format.sampleRate = info.hz;
        outClip.format.channels = static_cast<uint16_t>(info.channels);
        outClip.format.bitsPerSample = 16;

        size_t byteCount = info.samples * sizeof(mp3d_sample_t);
        outClip.data.resize(byteCount);
        memcpy(outClip.data.data(), info.buffer, byteCount);

        outClip.duration = static_cast<float>(info.samples / info.channels) / info.hz;

        free(info.buffer);
        return true;
    }

    static bool DecodeFile(const std::string& path, AudioClip& outClip) {
        mp3dec_t dec;
        mp3dec_init(&dec);

        mp3dec_file_info_t info = {};
        if (mp3dec_load(&dec, path.c_str(), &info, nullptr, nullptr)) {
            return false;
        }

        if (info.samples == 0 || info.channels == 0) {
            free(info.buffer);
            return false;
        }

        outClip.format.sampleRate = info.hz;
        outClip.format.channels = static_cast<uint16_t>(info.channels);
        outClip.format.bitsPerSample = 16;

        size_t byteCount = info.samples * sizeof(mp3d_sample_t);
        outClip.data.resize(byteCount);
        memcpy(outClip.data.data(), info.buffer, byteCount);

        outClip.duration = static_cast<float>(info.samples / info.channels) / info.hz;

        free(info.buffer);
        return true;
    }
};

} // namespace Prisma::Audio::Codecs
