using System;

namespace Prisma;

public enum BiquadFilterType
{
    Lowpass,
    Highpass,
    Bandpass,
    Notch,
    Peaking,
    LowShelf,
    HighShelf
}

public class BiquadFilterNode : DSPNode
{
    internal BiquadFilterNode(ulong handle, AudioGraph graph) : base(handle, graph) { }

    public BiquadFilterNode(AudioGraph graph, string name = null)
        : base(graph.CreateNode("BiquadFilter", name ?? "").Handle, graph) { }

    public float Frequency
    {
        get => GetParameter("frequency");
        set => SetParameter("frequency", value);
    }

    public float Q
    {
        get => GetParameter("q");
        set => SetParameter("q", value);
    }

    public float Gain
    {
        get => GetParameter("gain");
        set => SetParameter("gain", value);
    }

    public BiquadFilterType Type
    {
        get => (BiquadFilterType)(int)GetParameter("type");
        set => SetParameter("type", (float)(int)value);
    }
}
