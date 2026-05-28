#include <gtest/gtest.h>
#include <audio/dsp/AudioBuffer.h>
#include <audio/dsp/AudioNode.h>
#include <audio/dsp/TestRenderer.h>
#include <audio/dsp/nodes/OscillatorNode.h>
#include <audio/dsp/nodes/ADSRNode.h>
#include <audio/dsp/nodes/SVFNode.h>
#include <audio/dsp/nodes/DelayNode.h>
#include <audio/dsp/nodes/DistortionNode.h>
#include <audio/dsp/nodes/MasterBusNode.h>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <vector>

using namespace Prisma::Audio::DSP;

// ============================================================================
// Helper nodes (from the original main.cpp)
// ============================================================================

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

// ============================================================================
// Build a moderately complex graph to test determinism
// (from the original DeterministicTest.cpp)
// ============================================================================

static void BuildTestGraph(AudioGraph& graph) {
    auto osc = graph.CreateNode(std::make_unique<OscillatorNode>(OscillatorNode::Waveform::Square));
    osc->SetParameter("frequency", 220.0f);
    osc->SetParameter("amplitude", 0.5f);

    auto adsr = graph.CreateNode(std::make_unique<ADSRNode>());
    adsr->SetParameter("attack", 0.01f);
    adsr->SetParameter("decay", 0.1f);
    adsr->SetParameter("sustain", 0.6f);
    adsr->SetParameter("release", 0.2f);
    adsr->SetParameter("gate", 1.0f);

    auto filter = graph.CreateNode(std::make_unique<SVFNode>());
    filter->SetParameter("cutoff", 1000.0f);
    filter->SetParameter("resonance", 0.3f);
    filter->SetParameter("mode", 0.0f);

    auto delay = graph.CreateNode(std::make_unique<DelayNode>());
    delay->SetParameter("delayTime", 0.1f);
    delay->SetParameter("feedback", 0.3f);
    delay->SetParameter("mix", 0.5f);

    auto dist = graph.CreateNode(std::make_unique<DistortionNode>());
    dist->SetParameter("drive", 2.0f);
    dist->SetParameter("mix", 0.7f);

    auto master = graph.CreateNode(std::make_unique<MasterBusNode>());

    // ADSR input pin is "Gate", not "Input"
    graph.Connect(osc, "Output", adsr, "Gate");
    graph.Connect(adsr, "Output", filter, "Input");
    // SVF output pin is "Lowpass" when mode=0
    graph.Connect(filter, "Lowpass", delay, "Input");
    graph.Connect(delay, "Output", dist, "Input");
    graph.Connect(dist, "Output", master, "Input");
}

// ============================================================================
// Test 1 (from main.cpp): Constant source + gain, verify output is 0.5f
// ============================================================================

TEST(AudioDSP, ConstantPlusGain) {
    AudioGraph graph;
    auto src = graph.CreateNode(std::make_unique<ConstantSourceNode>(1.0f));
    auto gain = graph.CreateNode(std::make_unique<GainNode>());
    gain->SetParameter("gain", 0.5f);
    graph.Connect(src, "Output", gain, "Input");

    TestRenderer r;
    r.SetGraph(&graph);
    r.SetDuration(0.1f);
    auto result = r.Render();

    bool pass = true;
    for (auto s : result) {
        if (fabsf(s - 0.5f) > 0.001f) {
            pass = false;
            break;
        }
    }
    EXPECT_TRUE(pass) << "Constant+Gain output should be 0.5f";
}

// ============================================================================
// Test 2 (from main.cpp): Sine oscillator, render WAV, verify file size
// ============================================================================

TEST(AudioDSP, SineWavRender) {
    AudioGraph graph;
    auto osc = graph.CreateNode(std::make_unique<SineOscillatorNode>(440.0f));
    osc->SetParameter("amplitude", 0.5f);

    TestRenderer r;
    r.SetGraph(&graph);
    r.SetDuration(0.5f);
    bool wrote = r.RenderToFile("test_440.wav");

    FILE* f = fopen("test_440.wav", "rb");
    ASSERT_NE(f, nullptr) << "WAV file should exist";
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fclose(f);

    EXPECT_TRUE(wrote) << "RenderToFile should succeed";
    EXPECT_GT(sz, 44) << "WAV file should be larger than header (44 bytes)";
}

// ============================================================================
// Test 3 (from main.cpp): Same graph rendered twice must be bit-identical
// ============================================================================

TEST(AudioDSP, DeterministicRenderSimple) {
    AudioGraph graph;
    graph.CreateNode(std::make_unique<SineOscillatorNode>(440.0f));

    TestRenderer r;
    r.SetGraph(&graph);
    r.SetDuration(0.1f);
    auto a = r.Render();
    auto b = r.Render();

    ASSERT_EQ(a.size(), b.size()) << "Two renders should have same sample count";
    bool same = memcmp(a.data(), b.data(), a.size() * sizeof(float)) == 0;
    EXPECT_TRUE(same) << "Two renders of identical graph should be bit-identical";
}

// ============================================================================
// Test 4 (from DeterministicTest.cpp): Complex graph, 3 independent renders
// ============================================================================

TEST(AudioDSP, ComplexGraphDeterminism) {
    std::vector<float> renders[3];

    for (int run = 0; run < 3; run++) {
        AudioGraph graph(48000, 256);
        BuildTestGraph(graph);

        TestRenderer renderer;
        renderer.SetGraph(&graph);
        renderer.SetSampleRate(48000);
        renderer.SetDuration(0.5f);
        renders[run] = renderer.Render();
    }

    // Compare all runs
    for (int i = 1; i < 3; i++) {
        ASSERT_EQ(renders[0].size(), renders[i].size())
            << "Run 1 (" << renders[0].size() << " samples) vs Run "
            << (i + 1) << " (" << renders[i].size() << " samples): size mismatch";

        int diffs = 0;
        for (size_t j = 0; j < renders[0].size(); j++) {
            if (fabsf(renders[0][j] - renders[i][j]) > 1e-6f) {
                diffs++;
            }
        }

        EXPECT_EQ(diffs, 0)
            << "Run 1 vs Run " << (i + 1) << ": "
            << diffs << " differing samples (out of " << renders[0].size() << ")";
    }

    // Render to WAV as evidence (non-fatal)
    {
        AudioGraph graph(48000, 256);
        BuildTestGraph(graph);
        TestRenderer r;
        r.SetGraph(&graph);
        r.SetSampleRate(48000);
        r.SetDuration(1.0f);
        r.RenderToFile("test_deterministic.wav");
    }
}
