using System;

namespace Prisma;

public enum LfoWaveform
{
    Sine,
    Triangle,
    Square,
    Saw,
    SampleAndHold
}

public class LFONode : DSPNode
{
    internal LFONode(ulong handle, AudioGraph graph) : base(handle, graph) { }

    public LFONode(AudioGraph graph, string name = null)
        : base(graph.CreateNode("LFO", name ?? "").Handle, graph) { }

    public float Rate
    {
        get => GetParameter("rate");
        set => SetParameter("rate", value);
    }

    public float Depth
    {
        get => GetParameter("depth");
        set => SetParameter("depth", value);
    }

    public LfoWaveform Waveform
    {
        get => (LfoWaveform)(int)GetParameter("waveform");
        set => SetParameter("waveform", (float)(int)value);
    }

    public float Offset
    {
        get => GetParameter("offset");
        set => SetParameter("offset", value);
    }
}
