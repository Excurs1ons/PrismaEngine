#pragma once

#include <Engine/audio/AudioTypes.h>
#include <cstdint>
#include <vector>
#include <string>
#include <fstream>

namespace Prisma::Audio::Codecs {

class WavDecoder {
public:
    struct WavHeader {
        char riff[4] = {};
        uint32_t fileSize = 0;
        char wave[4] = {};
        char fmtId[4] = {};
        uint32_t fmtSize = 0;
        uint16_t formatTag = 0;
        uint16_t channels = 0;
        uint32_t sampleRate = 0;
        uint32_t byteRate = 0;
        uint16_t blockAlign = 0;
        uint16_t bitsPerSample = 0;
    };

    static bool IsWavFormat(const uint8_t* header, size_t size) {
        if (size < 12) return false;
        return header[0] == 'R' && header[1] == 'I' && header[2] == 'F' && header[3] == 'F' &&
               header[8] == 'W' && header[9] == 'A' && header[10] == 'V' && header[11] == 'E';
    }

    static bool Decode(const uint8_t* data, size_t size, AudioClip& outClip) {
        if (!data || size < 44) return false;

        WavHeader header;
        memcpy(&header, data, sizeof(WavHeader));

        if (memcmp(header.riff, "RIFF", 4) != 0 || memcmp(header.wave, "WAVE", 4) != 0)
            return false;

        // 解析 fmt chunk — 可能需要跳过扩展字段
        size_t offset = 12;
        bool foundFmt = false, foundData = false;
        uint32_t dataSize = 0;

        while (offset + 8 <= size) {
            char chunkId[5] = {};
            memcpy(chunkId, data + offset, 4);
            uint32_t chunkSize;
            memcpy(&chunkSize, data + offset + 4, 4);
            offset += 8;

            if (memcmp(chunkId, "fmt ", 4) == 0) {
                foundFmt = true;
                if (chunkSize >= 16) {
                    memcpy(&header.formatTag, data + offset, 2);
                    memcpy(&header.channels, data + offset + 2, 2);
                    memcpy(&header.sampleRate, data + offset + 4, 4);
                    memcpy(&header.byteRate, data + offset + 8, 4);
                    memcpy(&header.blockAlign, data + offset + 12, 2);
                    memcpy(&header.bitsPerSample, data + offset + 14, 2);
                }
            } else if (memcmp(chunkId, "data", 4) == 0) {
                foundData = true;
                dataSize = chunkSize;
                break;
            }

            offset += chunkSize;
            if (chunkSize % 2) offset++; // padding byte
        }

        if (!foundFmt || !foundData) return false;

        uint16_t effectiveBps = (header.bitsPerSample == 0) ? 16 : header.bitsPerSample;

        outClip.format.sampleRate = header.sampleRate;
        outClip.format.channels = header.channels;
        outClip.format.bitsPerSample = effectiveBps;

        outClip.data.resize(dataSize);
        memcpy(outClip.data.data(), data + offset, dataSize);

        uint32_t totalSamples = dataSize / (effectiveBps / 8);
        outClip.duration = static_cast<float>(totalSamples) / (header.sampleRate * header.channels);

        return true;
    }

    static bool DecodeFile(const std::string& path, AudioClip& outClip) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) return false;
        std::streamsize size = file.tellg();
        file.seekg(0);
        std::vector<uint8_t> buf(size);
        if (!file.read(reinterpret_cast<char*>(buf.data()), size)) return false;
        return Decode(buf.data(), buf.size(), outClip);
    }

    // 支持 WAV 格式:
    // 1 (PCM): 标准 PCM
    // 3 (IEEE float): 32-bit float
    static bool IsFormatSupported(uint16_t formatTag) {
        return formatTag == 1 || formatTag == 3;
    }
};

} // namespace Prisma::Audio::Codecs
