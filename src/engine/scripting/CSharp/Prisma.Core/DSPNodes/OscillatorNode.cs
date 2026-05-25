using System;

namespace Prisma;

public enum OscillatorWaveform
{
    Sine,
    Square,
    Saw,
    Triangle,
    Noise
}

public class OscillatorNode : DSPNode
{
    internal OscillatorNode(ulong handle, AudioGraph graph) : base(handle, graph) { }

    public OscillatorNode(AudioGraph graph, string name = null)
        : base(graph.CreateNode("Oscillator", name ?? "").Handle, graph) { }

    public float Frequency
    {
        get => GetParameter("frequency");
        set => SetParameter("frequency", value);
    }

    public float Amplitude
    {
        get => GetParameter("amplitude");
        set => SetParameter("amplitude", value);
    }

    public float PulseWidth
    {
        get => GetParameter("pulseWidth");
        set => SetParameter("pulseWidth", value);
    }

    public OscillatorWaveform Waveform
    {
        get => (OscillatorWaveform)(int)GetParameter("waveform");
        set => SetParameter("waveform", (float)(int)value);
    }
}
