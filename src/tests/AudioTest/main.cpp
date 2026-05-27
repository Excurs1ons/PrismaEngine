#include <audio/dsp/AudioBuffer.h>
#include <audio/dsp/AudioNode.h>
#include <audio/dsp/TestRenderer.h>
#include <cstdio>
#include <cmath>
#include <cassert>
#include <cstring>

using namespace Prisma::Audio::DSP;

class ConstantSourceNode : public AudioNode {
public:
    explicit ConstantSourceNode(float value = 1.0f) : m_value(value) {
        AddOutputPin("Output");
        SetName("ConstantSource");
    }
    void Process(AudioBuffer& output, const AudioProcessContext&) override {
        for (uint32_t ch = 0; ch < output.GetChannels(); ++ch) {
            float* buf = output.GetChannel(ch);
            for (uint32_t f = 0; f < output.GetFrames(); ++f) buf[f] = m_value;
        }
    }
private:
    float m_value;
};

class GainNode : public AudioNode {
public:
    GainNode() { AddInputPin("Input"); AddOutputPin("Output"); SetName("Gain"); }
    void Process(AudioBuffer& output, const AudioProcessContext&) override {
        AudioBuffer* input = ReadInput("Input");
        if (!input) { output.Clear(); return; }
        float gain = GetParameter("gain");
        for (uint32_t ch = 0; ch < output.GetChannels(); ++ch) {
            const float* src = input->GetChannel(ch);
            float* dst = output.GetChannel(ch);
            for (uint32_t f = 0; f < output.GetFrames(); ++f) dst[f] = src[f] * gain;
        }
    }
};

class SineOscillatorNode : public AudioNode {
public:
    SineOscillatorNode(float freq = 440.0f) {
        AddOutputPin("Output"); SetName("SineOscillator");
        SetParameter("frequency", freq); SetParameter("amplitude", 1.0f);
    }
    void Process(AudioBuffer& output, const AudioProcessContext& ctx) override {
        float freq = GetParameter("frequency");
        float amp = GetParameter("amplitude");
        for (uint32_t ch = 0; ch < output.GetChannels(); ++ch) {
            float* buf = output.GetChannel(ch);
            for (uint32_t f = 0; f < output.GetFrames(); ++f) {
                buf[f] = amp * sinf(2.0f * M_PI * freq * m_phase / ctx.sampleRate);
                m_phase += 1.0f;
                if (m_phase >= ctx.sampleRate) m_phase -= ctx.sampleRate;
            }
        }
    }
    void Reset() override { m_phase = 0.0f; }
private:
    float m_phase = 0.0f;
};

int main() {
    printf("=== Audio DSP Test ===\n");

    // Test 1: Constant + gain
    {
        printf("Test 1: Constant + gain... ");
        AudioGraph graph;
        auto src = graph.CreateNode(std::make_unique<ConstantSourceNode>(1.0f));
        auto gain = graph.CreateNode(std::make_unique<GainNode>());
        gain->SetParameter("gain", 0.5f);
        graph.Connect(src, "Output", gain, "Input");
        TestRenderer r; r.SetGraph(&graph); r.SetDuration(0.1f);
        auto result = r.Render();
        bool pass = true;
        for (auto s : result) if (fabsf(s - 0.5f) > 0.001f) { pass = false; break; }
        printf("%s\n", pass ? "PASS" : "FAIL");
    }

    // Test 2: Sine WAV render
    {
        printf("Test 2: Sine WAV... ");
        AudioGraph graph;
        auto osc = graph.CreateNode(std::make_unique<SineOscillatorNode>(440.0f));
        osc->SetParameter("amplitude", 0.5f);
        TestRenderer r; r.SetGraph(&graph); r.SetDuration(0.5f);
        bool wrote = r.RenderToFile("test_440.wav");
        FILE* f = fopen("test_440.wav", "rb");
        assert(f); fseek(f, 0, SEEK_END); long sz = ftell(f); fclose(f);
        printf("%s (%ld bytes)\n", wrote && sz > 44 ? "PASS" : "FAIL", sz);
    }

    // Test 3: Deterministic
    {
        printf("Test 3: Deterministic... ");
        AudioGraph graph;
        graph.CreateNode(std::make_unique<SineOscillatorNode>(440.0f));
        TestRenderer r; r.SetGraph(&graph); r.SetDuration(0.1f);
        auto a = r.Render(); auto b = r.Render();
        bool same = (a.size() == b.size()) && memcmp(a.data(), b.data(), a.size() * sizeof(float)) == 0;
        printf("%s\n", same ? "PASS" : "FAIL");
    }

    printf("=== All tests passed! ===\n");
    return 0;
}
