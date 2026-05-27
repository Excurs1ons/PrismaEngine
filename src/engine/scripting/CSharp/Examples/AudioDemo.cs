using System;
using Prisma;

namespace PrismaCraft.Demos;

/// <summary>
/// Demonstrates the complete PrismaEngine C# Audio API.
/// Shows graph construction, DSP node control, bus routing, and spectrum analysis.
/// Attach this to a world script or call from Bootstrap.
/// </summary>
public static class AudioDemo
{
    private static AudioGraph s_graph;
    private static AudioMixer s_mixer;
    private static AudioPlayer s_player;
    private static SpectrumAnalyzerNode s_spectrum;
    private static OscillatorNode s_leadOsc;
    private static ADSRNode s_adsr;
    private static SVFilterNode s_filter;
    private static ReverbNode s_reverb;
    private static float s_time;

    /// <summary>
    /// Called during Bootstrap to initialize the audio pipeline.
    /// </summary>
    public static void Init()
    {
        Debug.Log("[AudioDemo] Initializing audio pipeline...");

        // 1. Create AudioGraph — the DSP processing engine
        s_graph = new AudioGraph(48000, 256);

        // 2. Create AudioMixer for bus management
        s_mixer = new AudioMixer(s_graph);
        var masterBus = s_mixer.MasterBus;
        masterBus.Volume = 0.8f;

        // 3. Create synth voice chain:
        //    Oscillator -> ADSR -> SVFilter -> Master
        s_leadOsc = new OscillatorNode(s_graph, "LeadOsc");
        s_leadOsc.Waveform   = OscillatorWaveform.Saw;
        s_leadOsc.Frequency  = 440.0f;
        s_leadOsc.Amplitude  = 0.5f;

        s_adsr = new ADSRNode(s_graph, "LeadADSR");
        s_adsr.Attack  = 0.05f;
        s_adsr.Decay   = 0.10f;
        s_adsr.Sustain = 0.70f;
        s_adsr.Release = 0.30f;
        s_adsr.Gate    = 1.0f;

        s_filter = new SVFilterNode(s_graph, "LeadFilter");
        s_filter.Mode     = SVFMode.Lowpass;
        s_filter.Cutoff   = 2000.0f;
        s_filter.Resonance = 0.3f;

        // Connect: Osc -> ADSR -> Filter -> Master
        s_graph.Connect(s_leadOsc, "Output", s_adsr, "Input");
        s_graph.Connect(s_adsr,    "Output", s_filter, "Input");
        s_mixer.RouteToMaster(s_filter);

        // 4. Create reverb send effect
        s_reverb = new ReverbNode(s_graph, "MainReverb");
        s_reverb.RoomSize = 0.6f;
        s_reverb.Damping  = 0.5f;
        s_reverb.Mix      = 0.3f;

        // Route: Reverb -> Master (as send effect)
        s_mixer.RouteToMaster(s_reverb);

        // 5. Create Spectrum Analyzer for visualization
        s_spectrum = new SpectrumAnalyzerNode(2048);

        // 6. AudioPlayer for triggering one-shot sounds
        s_player = new AudioPlayer();

        Debug.Log("[AudioDemo] Audio pipeline ready!");
    }

    /// <summary>
    /// Called every frame (from OnFrame).
    /// </summary>
    public static void Update(float dt)
    {
        s_time += dt;

        // Modulate filter cutoff with LFO-like pattern
        float lfo = 0.5f + 0.5f * MathF.Sin(s_time * 0.5f);
        float cutoff = 500.0f + 4000.0f * lfo;
        s_filter.Cutoff = cutoff;

        // Every 4 seconds, retrigger ADSR
        float prev = s_time - dt;
        if ((int)(prev / 4.0f) != (int)(s_time / 4.0f))
        {
            s_adsr.Gate = 1.0f;
            Debug.Log("[AudioDemo] Note on!");
        }

        // Spectrum visualization (for external display)
        // SpectrumAnalyzerNode.SpectrumBin[] bins = ReadSpectrum();
    }

    /// <summary>
    /// Read spectrum data for visualization.
    /// </summary>
    public static SpectrumAnalyzerNode.SpectrumBin[] ReadSpectrum()
    {
        return s_spectrum?.GetSpectrum() ?? Array.Empty<SpectrumAnalyzerNode.SpectrumBin>();
    }

    /// <summary>
    /// Cleanup on shutdown.
    /// </summary>
    public static void Shutdown()
    {
        s_player?.Dispose();
        s_spectrum?.Dispose();
        s_graph?.Dispose();
        // s_mixer nodes and s_reverb are owned by the graph — disposed via s_graph

        Debug.Log("[AudioDemo] Audio shutdown complete.");
    }
}
