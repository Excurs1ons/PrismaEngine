#include <audio/dsp/AudioNode.h>
#include <audio/dsp/TestRenderer.h>
#include <audio/dsp/nodes/OscillatorNode.h>
#include <audio/dsp/nodes/ADSRNode.h>
#include <audio/dsp/nodes/SVFNode.h>
#include <audio/dsp/nodes/MasterBusNode.h>
#include <cstdio>
#include <cmath>
#include <chrono>
#include <vector>
#include <memory>
#include <cstring>

using namespace Prisma::Audio::DSP;
using namespace std::chrono;

int main() {
    printf("=== Audio Stress Test ===\n");
    int failures = 0;

    // Test 1: 128 voices x 3 effects => ~400 nodes
    {
        printf("Stress 1: 128 voices x 3 effects...\n");
        AudioGraph graph(48000, 256);

        auto master = graph.CreateNode(std::make_unique<MasterBusNode>());

        for (int i = 0; i < 128; i++) {
            float freq = 220.0f + static_cast<float>(i % 48) * 5.0f;

            auto osc = graph.CreateNode(std::make_unique<OscillatorNode>(OscillatorNode::Waveform::Saw));
            osc->SetParameter("frequency", freq);
            osc->SetParameter("amplitude", 0.1f);

            auto adsr = graph.CreateNode(std::make_unique<ADSRNode>());
            adsr->SetParameter("attack", 0.01f);
            adsr->SetParameter("decay", 0.05f);
            adsr->SetParameter("sustain", 0.5f);
            adsr->SetParameter("release", 0.1f);
            adsr->SetParameter("gate", 1.0f);

            auto filter = graph.CreateNode(std::make_unique<SVFNode>());
            filter->SetParameter("cutoff", freq * 3.0f);
            filter->SetParameter("resonance", 0.2f);
            filter->SetParameter("mode", 0.0f); // Lowpass

            // ADSR input pin is "Gate", not "Input"
            graph.Connect(osc, "Output", adsr, "Gate");
            graph.Connect(adsr, "Output", filter, "Input");
            // SVF output pin is "Lowpass" when mode=0
            graph.Connect(filter, "Lowpass", master, "Input");
        }

        auto start = high_resolution_clock::now();
        TestRenderer renderer;
        renderer.SetGraph(&graph);
        renderer.SetDuration(2.0f);
        // Ensure sample rate matches graph
        renderer.SetSampleRate(48000);

        auto samples = renderer.Render();
        auto end = high_resolution_clock::now();

        auto elapsed = duration_cast<milliseconds>(end - start).count();
        float sampleCount = static_cast<float>(samples.size());
        float sampleRate = 48000.0f;
        float expectedSamples = sampleRate * 2.0f * 2.0f; // stereo frames * channels

        // Check output has audio content
        bool hasAudio = false;
        for (size_t i = 0; i < samples.size() && !hasAudio; i++) {
            if (fabsf(samples[i]) > 0.001f) hasAudio = true;
        }

        printf("   %zu samples in %ldms (%.1fx realtime)\n",
               samples.size(), elapsed,
               elapsed > 0 ? (2000.0f / elapsed) : 0.0f);
        printf("   Has Audio: %s\n", hasAudio ? "YES" : "NO (all silent)");

        if (!hasAudio) {
            printf("   FAIL (no audio output)\n");
            failures++;
        } else {
            printf("   PASS\n");
        }
    }

    // Test 2: 1000 graph create/destroy cycles
    {
        printf("Stress 2: 1000 graph create/destroy cycles... ");
        fflush(stdout);

        auto start = high_resolution_clock::now();

        for (int i = 0; i < 1000; i++) {
            AudioGraph g(48000, 256);
            auto osc = g.CreateNode(std::make_unique<OscillatorNode>(OscillatorNode::Waveform::Sine));
            auto master = g.CreateNode(std::make_unique<MasterBusNode>());
            g.Connect(osc, "Output", master, "Input");
            // Graph goes out of scope and is destroyed
        }

        auto end = high_resolution_clock::now();
        auto elapsed = duration_cast<milliseconds>(end - start).count();
        printf("%ldms (%d cycles)\n", elapsed, 1000);
        printf("   PASS\n");
    }

    printf("=== Stress Test Complete: %d failures ===\n", failures);
    return failures;
}
