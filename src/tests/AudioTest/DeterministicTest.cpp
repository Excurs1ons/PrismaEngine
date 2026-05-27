#include <audio/dsp/AudioNode.h>
#include <audio/dsp/TestRenderer.h>
#include <audio/dsp/nodes/OscillatorNode.h>
#include <audio/dsp/nodes/ADSRNode.h>
#include <audio/dsp/nodes/SVFNode.h>
#include <audio/dsp/nodes/DelayNode.h>
#include <audio/dsp/nodes/DistortionNode.h>
#include <audio/dsp/nodes/MasterBusNode.h>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <vector>

using namespace Prisma::Audio::DSP;

// Build a moderately complex graph to test determinism
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

int main() {
    printf("=== Deterministic Test ===\n");
    int failures = 0;

    // Generate 3 separate renders from identical graphs
    std::vector<float> renders[3];

    for (int run = 0; run < 3; run++) {
        AudioGraph graph(48000, 256);
        BuildTestGraph(graph);

        TestRenderer renderer;
        renderer.SetGraph(&graph);
        renderer.SetSampleRate(48000);
        renderer.SetDuration(0.5f);
        renders[run] = renderer.Render();

        printf("Run %d: %zu samples\n", run + 1, renders[run].size());
    }

    // Compare all runs
    bool allMatch = true;
    for (int i = 1; i < 3; i++) {
        if (renders[0].size() != renders[i].size()) {
            printf("Size mismatch: run1=%zu run%d=%zu\n",
                   renders[0].size(), i + 1, renders[i].size());
            allMatch = false;
            continue;
        }

        int diffs = 0;
        for (size_t j = 0; j < renders[0].size(); j++) {
            if (fabsf(renders[0][j] - renders[i][j]) > 1e-6f) {
                diffs++;
                if (diffs <= 5) {
                    printf("  Diff at sample %zu: %.8f vs %.8f\n",
                           j, renders[0][j], renders[i][j]);
                }
            }
        }

        if (diffs > 0) {
            printf("Run 1 vs Run %d: %d differing samples (out of %zu) -- DIFFERENT\n",
                   i + 1, diffs, renders[0].size());
            allMatch = false;
        }
    }

    if (allMatch) {
        printf("Deterministic: PASS (all %zu samples identical across 3 runs)\n",
               renders[0].size());
    } else {
        printf("Deterministic: FAIL\n");
        failures++;
    }

    // Render to WAV as evidence
    {
        AudioGraph graph(48000, 256);
        BuildTestGraph(graph);
        TestRenderer r;
        r.SetGraph(&graph);
        r.SetSampleRate(48000);
        r.SetDuration(1.0f);
        r.RenderToFile("test_deterministic.wav");
        printf("Evidence WAV: test_deterministic.wav\n");
    }

    printf("=== Deterministic Test Complete: %d failures ===\n", failures);
    return failures;
}
