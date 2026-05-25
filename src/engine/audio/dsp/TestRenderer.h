#pragma once

#include "AudioBuffer.h"
#include "AudioNode.h"

#include <vector>
#include <string>
#include <cstdint>
#include <fstream>
#include <cmath>
#include <cstring>
#include <algorithm>

namespace Prisma::Audio::DSP {

enum class WavFormat : uint8_t {
    Pcm16   = 0,
    Float32 = 1,
};

namespace detail {

struct WavInfo {
    uint32_t    sampleRate    = 0;
    uint16_t    channels      = 0;
    uint16_t    bitsPerSample = 0;
    uint32_t    dataSize      = 0;
    std::vector<float> samples;
};

inline bool ReadWavFile(const std::string& path, WavInfo& info)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return false;

    char riff[4]{};
    file.read(riff, 4);
    if (std::memcmp(riff, "RIFF", 4) != 0) return false;
    file.seekg(4, std::ios::cur);
    char wave[4]{};
    file.read(wave, 4);
    if (std::memcmp(wave, "WAVE", 4) != 0) return false;

    uint16_t audioFormat = 0;
    bool foundFmt = false, foundData = false;

    while (file.peek() != EOF) {
        char chunkId[4]{};
        file.read(chunkId, 4);
        uint32_t chunkSize = 0;
        file.read(reinterpret_cast<char*>(&chunkSize), 4);

        if (std::memcmp(chunkId, "fmt ", 4) == 0) {
            if (chunkSize < 16) return false;
            file.read(reinterpret_cast<char*>(&audioFormat), 2);
            file.read(reinterpret_cast<char*>(&info.channels), 2);
            file.read(reinterpret_cast<char*>(&info.sampleRate), 4);
            file.seekg(6, std::ios::cur);
            file.read(reinterpret_cast<char*>(&info.bitsPerSample), 2);
            foundFmt = true;
            if (chunkSize > 16) file.seekg(chunkSize - 16, std::ios::cur);
        } else if (std::memcmp(chunkId, "data", 4) == 0) {
            info.dataSize = chunkSize;
            foundData = true;
            break;
        } else {
            file.seekg(chunkSize, std::ios::cur);
        }
    }

    if (!foundFmt || !foundData) return false;
    if (audioFormat != 1 && audioFormat != 3) return false;

    const uint32_t totalSamples = info.dataSize / (info.bitsPerSample / 8);
    info.samples.resize(totalSamples);
    std::vector<uint8_t> raw(info.dataSize);
    file.read(reinterpret_cast<char*>(raw.data()), info.dataSize);

    if (audioFormat == 3 && info.bitsPerSample == 32) {
        const float* src = reinterpret_cast<const float*>(raw.data());
        std::copy(src, src + totalSamples, info.samples.data());
    } else if (audioFormat == 1 && info.bitsPerSample == 16) {
        const int16_t* src = reinterpret_cast<const int16_t*>(raw.data());
        for (uint32_t i = 0; i < totalSamples; ++i)
            info.samples[i] = static_cast<float>(src[i]) / 32768.0f;
    } else if (audioFormat == 1 && info.bitsPerSample == 8) {
        const uint8_t* src = raw.data();
        for (uint32_t i = 0; i < totalSamples; ++i)
            info.samples[i] = (static_cast<float>(src[i]) - 128.0f) / 128.0f;
    } else {
        return false;
    }

    return true;
}

} // namespace detail

class TestRenderer {
public:
    TestRenderer() = default;

    void SetSampleRate(uint32_t rate) { m_sampleRate = rate; }
    void SetFramesPerBlock(uint32_t frames) { m_framesPerBlock = frames; }
    void SetDuration(float seconds) { m_duration = seconds; }
    void SetGraph(AudioGraph* graph) { m_graph = graph; }
    void SetOutputFormat(WavFormat fmt) { m_wavFormat = fmt; }

    bool RenderToFile(const std::string& path)
    {
        std::vector<float> samples = Render();
        if (samples.empty()) return false;

        std::vector<uint8_t> wavData;
        WriteWav(wavData, samples, m_sampleRate, 2);

        std::ofstream file(path, std::ios::binary);
        if (!file.is_open()) return false;
        file.write(reinterpret_cast<const char*>(wavData.data()),
                   static_cast<std::streamsize>(wavData.size()));
        return file.good();
    }

    std::vector<float> Render()
    {
        if (!m_graph) return {};

        const uint64_t totalFrames = static_cast<uint64_t>(m_sampleRate * m_duration);
        if (totalFrames == 0) return {};

        const uint32_t blockSize = m_framesPerBlock;
        const uint32_t channels  = 2;

        std::vector<float> output(static_cast<size_t>(totalFrames) * channels, 0.0f);

        m_graph->Reset();

        uint64_t framesWritten = 0;
        while (framesWritten < totalFrames) {
            AudioBuffer blockBuf(channels, blockSize);
            blockBuf.Clear();

            m_graph->Process(blockBuf);

            const uint32_t toCopy = static_cast<uint32_t>(
                std::min(static_cast<uint64_t>(blockSize), totalFrames - framesWritten));

            for (uint32_t ch = 0; ch < channels; ++ch) {
                const float* src = blockBuf.GetChannel(ch);
                for (uint32_t f = 0; f < toCopy; ++f) {
                    output[static_cast<size_t>(framesWritten + f) * channels + ch] = src[f];
                }
            }

            framesWritten += toCopy;
        }

        return output;
    }

    bool CompareWithReference(const std::string& refPath, float toleranceDb = -60.0f)
    {
        detail::WavInfo refInfo;
        if (!detail::ReadWavFile(refPath, refInfo)) return false;
        if (refInfo.samples.empty()) return false;

        std::vector<float> rendered = Render();
        if (rendered.empty()) return false;

        const size_t compareLen = std::min(rendered.size(), refInfo.samples.size());

        double sumSqError = 0.0;
        for (size_t i = 0; i < compareLen; ++i) {
            double err = static_cast<double>(rendered[i])
                       - static_cast<double>(refInfo.samples[i]);
            sumSqError += err * err;
        }
        double rmsError = std::sqrt(sumSqError / static_cast<double>(compareLen));

        float errorDb = (rmsError <= 0.0) ? -144.0f
            : 20.0f * std::log10(static_cast<float>(rmsError));

        return errorDb <= toleranceDb;
    }

private:
    void WriteWav(std::vector<uint8_t>& dest, const std::vector<float>& samples,
                  uint32_t sampleRate, uint16_t channels)
    {
        if (samples.empty() || channels == 0 || sampleRate == 0) return;

        const uint32_t totalFrames   = static_cast<uint32_t>(samples.size() / channels);
        const uint16_t bitsPerSample = (m_wavFormat == WavFormat::Float32) ? 32 : 16;
        const uint16_t blockAlign    = channels * (bitsPerSample / 8);
        const uint32_t byteRate      = sampleRate * blockAlign;
        const uint32_t dataSize      = totalFrames * blockAlign;

        auto write32 = [&](uint32_t v) {
            dest.push_back(static_cast<uint8_t>(v & 0xFF));
            dest.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
            dest.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
            dest.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
        };
        auto write16 = [&](uint16_t v) {
            dest.push_back(static_cast<uint8_t>(v & 0xFF));
            dest.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
        };
        auto writeStr = [&](const char* s, size_t n) {
            dest.insert(dest.end(), s, s + n);
        };

        writeStr("RIFF", 4);
        write32(36 + dataSize);
        writeStr("WAVE", 4);
        writeStr("fmt ", 4);
        write32(16);
        write16(static_cast<uint16_t>(m_wavFormat == WavFormat::Float32 ? 3 : 1));
        write16(channels);
        write32(sampleRate);
        write32(byteRate);
        write16(blockAlign);
        write16(bitsPerSample);
        writeStr("data", 4);
        write32(dataSize);

        dest.reserve(dest.size() + dataSize);

        if (m_wavFormat == WavFormat::Float32) {
            for (float s : samples) {
                float clamped = std::clamp(s, -1.0f, 1.0f);
                uint32_t val;
                std::memcpy(&val, &clamped, sizeof(val));
                write32(val);
            }
        } else {
            for (float s : samples) {
                float clamped = std::clamp(s, -1.0f, 1.0f);
                int16_t val = static_cast<int16_t>(std::round(clamped * 32767.0f));
                write16(static_cast<uint16_t>(val));
            }
        }
    }

    AudioGraph* m_graph           = nullptr;
    uint32_t    m_sampleRate      = 48000;
    uint32_t    m_framesPerBlock  = 256;
    float       m_duration        = 1.0f;
    WavFormat   m_wavFormat       = WavFormat::Pcm16;
};

} // namespace Prisma::Audio::DSP
