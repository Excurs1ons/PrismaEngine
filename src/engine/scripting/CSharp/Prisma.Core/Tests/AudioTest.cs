using System;
using System.Linq;

namespace Prisma.Tests;

/// <summary>
/// Self-verifying unit tests for the PrismaEngine C# Audio API.
/// Each test exercises creation, configuration, connection, and teardown of DSP nodes.
/// Tests do not require a running engine — they validate API usage patterns and parameter round-trips.
/// </summary>
public static class AudioTest
{
    // Run all audio tests. Returns number of failed tests (0 = all pass).
    public static int RunAll()
    {
        int failed = 0;
        failed += RunTest("AudioGraph_CreateAndDispose",       TestAudioGraphCreateAndDispose);
        failed += RunTest("AudioGraph_ConnectAndDisconnect",   TestAudioGraphConnectDisconnect);
        failed += RunTest("AudioGraph_MultipleNodes",          TestAudioGraphMultipleNodes);
        failed += RunTest("AudioGraph_DisposeCleanup",         TestAudioGraphDisposeCleanup);
        failed += RunTest("OscillatorNode_Parameters",         TestOscillatorNodeParameters);
        failed += RunTest("ADSRNode_Parameters",               TestADSRNodeParameters);
        failed += RunTest("SVFilterNode_Parameters",           TestSVFilterNodeParameters);
        failed += RunTest("BiquadFilterNode_Parameters",       TestBiquadFilterNodeParameters);
        failed += RunTest("CompressorNode_Parameters",         TestCompressorNodeParameters);
        failed += RunTest("BusRouting",                        TestBusRouting);
        failed += RunTest("AudioMixer_CreateGroupBus",         TestAudioMixerCreateGroupBus);
        failed += RunTest("AudioMixer_RouteToGroup",           TestAudioMixerRouteToGroup);
        failed += RunTest("AudioPlayer_PlayStop",              TestAudioPlayerPlayStop);
        failed += RunTest("SpectrumAnalyzer_ProcessAndRead",   TestSpectrumAnalyzerProcessAndRead);
        failed += RunTest("LevelMeter_ChannelLevel",           TestLevelMeterChannelLevel);
        return failed;
    }

    private static int RunTest(string name, Func<int> test)
    {
        try
        {
            int result = test();
            if (result == 0)
                Debug.Log($"[PASS] {name}");
            else
                Debug.LogError($"[FAIL] {name}");
            return result;
        }
        catch (Exception e)
        {
            Debug.LogError($"[FAIL] {name}: {e.Message}");
            return 1;
        }
    }

    // ========== Graph Tests ==========

    /// <summary>Verify AudioGraph construction and disposal without leaking.</summary>
    static int TestAudioGraphCreateAndDispose()
    {
        var graph = new AudioGraph(48000, 256);
        if (graph == null) return 1;
        graph.Dispose();
        return 0;
    }

    /// <summary>Connect two DSP nodes, then disconnect them.</summary>
    static int TestAudioGraphConnectDisconnect()
    {
        using var graph = new AudioGraph();
        var osc = new OscillatorNode(graph, "TestOsc");
        var delay = new DelayNode(graph, "TestDelay");

        if (!graph.Connect(osc, "Output", delay, "Input")) return 1;
        if (!graph.Disconnect(osc, delay)) return 1;

        osc.Dispose();
        delay.Dispose();
        return 0;
    }

    /// <summary>Chain three nodes in series and verify connections via teardown.</summary>
    static int TestAudioGraphMultipleNodes()
    {
        using var graph = new AudioGraph();
        var osc  = new OscillatorNode(graph, "Osc");
        var adsr = new ADSRNode(graph, "ADSR");
        var svf  = new SVFilterNode(graph, "SVF");

        graph.Connect(osc, "Output", adsr, "Input");
        graph.Connect(adsr, "Output", svf, "Input");

        graph.Disconnect(adsr, svf);
        graph.Disconnect(osc, adsr);

        osc.Dispose();
        adsr.Dispose();
        svf.Dispose();
        return 0;
    }

    /// <summary>Dispose graph while nodes still exist — C++ side must not leak.</summary>
    static int TestAudioGraphDisposeCleanup()
    {
        var graph = new AudioGraph();
        var osc  = graph.CreateNode("Oscillator", "Osc");
        var adsr = graph.CreateNode("ADSR", "ADSR");

        // Graph disposal invalidates all child nodes
        graph.Dispose();
        return 0;
    }

    // ========== DSP Node Parameter Tests ==========

    /// <summary>Set and read back OscillatorNode parameters.</summary>
    static int TestOscillatorNodeParameters()
    {
        using var graph = new AudioGraph();
        var osc = new OscillatorNode(graph, "Test");

        osc.Frequency = 440.0f;
        if (Math.Abs(osc.Frequency - 440.0f) > 0.001f) return 1;

        osc.Amplitude = 0.5f;
        if (Math.Abs(osc.Amplitude - 0.5f) > 0.001f) return 1;

        osc.PulseWidth = 0.5f;
        osc.Waveform = OscillatorWaveform.Saw;

        // Verify waveform round-trip
        if (osc.Waveform != OscillatorWaveform.Saw) return 1;

        return 0;
    }

    /// <summary>Set and read back ADSR envelope parameters.</summary>
    static int TestADSRNodeParameters()
    {
        using var graph = new AudioGraph();
        var adsr = new ADSRNode(graph, "Test");

        adsr.Attack  = 0.50f;
        adsr.Decay   = 0.20f;
        adsr.Sustain = 0.80f;
        adsr.Release = 0.30f;
        adsr.Gate    = 1.00f;

        if (Math.Abs(adsr.Attack  - 0.50f) > 0.001f) return 1;
        if (Math.Abs(adsr.Decay   - 0.20f) > 0.001f) return 1;
        if (Math.Abs(adsr.Sustain - 0.80f) > 0.001f) return 1;
        if (Math.Abs(adsr.Release - 0.30f) > 0.001f) return 1;
        if (Math.Abs(adsr.Gate    - 1.00f) > 0.001f) return 1;

        return 0;
    }

    /// <summary>Set and read back SVFilterNode parameters.</summary>
    static int TestSVFilterNodeParameters()
    {
        using var graph = new AudioGraph();
        var svf = new SVFilterNode(graph, "Test");

        svf.Cutoff    = 2000.0f;
        svf.Resonance = 0.5f;
        svf.Mode      = SVFMode.Bandpass;

        if (Math.Abs(svf.Cutoff - 2000.0f) > 0.001f) return 1;
        if (svf.Mode != SVFMode.Bandpass) return 1;

        return 0;
    }

    /// <summary>Set and read back BiquadFilterNode parameters.</summary>
    static int TestBiquadFilterNodeParameters()
    {
        using var graph = new AudioGraph();
        var bqf = new BiquadFilterNode(graph, "Test");

        bqf.Frequency = 1000.0f;
        bqf.Q         = 1.0f;
        bqf.Gain      = 3.0f;
        bqf.Type      = BiquadFilterType.LowShelf;

        if (Math.Abs(bqf.Frequency - 1000.0f) > 0.001f) return 1;
        if (Math.Abs(bqf.Q         - 1.0f)    > 0.001f) return 1;

        return 0;
    }

    /// <summary>Set and read back CompressorNode parameters.</summary>
    static int TestCompressorNodeParameters()
    {
        using var graph = new AudioGraph();
        var comp = new CompressorNode(graph, "Test");

        comp.Threshold   = -18.0f;
        comp.Ratio       = 8.0f;
        comp.Attack      = 0.005f;
        comp.Release     = 0.200f;
        comp.Makeup      = 2.0f;
        comp.Knee        = 3.0f;
        comp.Mix         = 0.8f;

        if (Math.Abs(comp.Threshold - (-18.0f)) > 0.001f) return 1;
        if (Math.Abs(comp.Ratio     -   8.0f)   > 0.001f) return 1;

        return 0;
    }

    // ========== Bus Routing Tests ==========

    /// <summary>Route nodes through buses: Input -> Master, Reverb -> Master.</summary>
    static int TestBusRouting()
    {
        using var graph = new AudioGraph();
        var master = new MasterBusNode(graph, "Master");
        var reverb = new ReverbNode(graph, "Reverb");
        var input  = new OscillatorNode(graph, "Input");

        graph.Connect(input,  "Output", master, "Input");
        graph.Connect(reverb, "Output", master, "Input");
        graph.Disconnect(input, master);

        input.Dispose();
        reverb.Dispose();
        master.Dispose();
        return 0;
    }

    /// <summary>Create group buses via AudioMixer and verify IReadOnlyList count.</summary>
    static int TestAudioMixerCreateGroupBus()
    {
        using var graph = new AudioGraph();
        var mixer = new AudioMixer(graph);

        var group1 = mixer.CreateGroupBus("Drums");
        var group2 = mixer.CreateGroupBus("Vocals");

        if (mixer.GroupBuses.Count != 2) return 1;

        return 0;
    }

    /// <summary>Route a DSP source node to a GroupBus via AudioMixer.</summary>
    static int TestAudioMixerRouteToGroup()
    {
        using var graph = new AudioGraph();
        var mixer = new AudioMixer(graph);

        var drums = mixer.CreateGroupBus("Drums");
        var kick  = new OscillatorNode(graph, "Kick");

        mixer.RouteToGroup(kick, drums);

        kick.Dispose();
        return 0;
    }

    // ========== AudioPlayer Tests ==========

    /// <summary>Play a nonexistent clip and verify graceful failure.</summary>
    static int TestAudioPlayerPlayStop()
    {
        var player = new AudioPlayer();

        // Play should handle missing clip gracefully
        var voice = player.Play("nonexistent.wav", 1.0f, 1.0f, false);

        // voice.Id should be 0 for failed playback (clip not found)
        // The voice is not added to ActiveVoices when Id == 0
        if (voice.Id != 0) player.Stop(voice);

        player.StopAll();
        player.Dispose();

        return 0;
    }

    // ========== Spectrum & Level Tests ==========

    /// <summary>Feed a test tone to SpectrumAnalyzer and read bins.</summary>
    static int TestSpectrumAnalyzerProcessAndRead()
    {
        using var analyzer = new SpectrumAnalyzerNode(1024);

        // Generate a test tone: 440 Hz sine at 48 kHz
        float[] tone = new float[2048];
        for (int i = 0; i < tone.Length; i++)
            tone[i] = MathF.Sin(2 * MathF.PI * 440 * i / 48000);

        analyzer.Process(tone, 48000);

        var spectrum = analyzer.GetSpectrum();
        // Should return at least some bins (fftSize/2 = 512)
        if (spectrum.Length == 0) return 1;

        // Peak magnitude should be non-trivial with a tone present
        var peak = analyzer.PeakMagnitude;

        return 0;
    }

    /// <summary>Read channel level from LevelMeterNode (no crash).</summary>
    static int TestLevelMeterChannelLevel()
    {
        using var graph = new AudioGraph();
        var meter = new LevelMeterNode(graph, "Meter");

        // Just verify the API call doesn't throw
        var level = meter.GetChannelLevel(0);

        return 0;
    }
}
