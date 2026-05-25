using System;

namespace Prisma;

public enum SVFMode
{
    Lowpass,
    Highpass,
    Bandpass,
    Notch,
    Peak,
    Allpass
}

public class SVFilterNode : DSPNode
{
    internal SVFilterNode(ulong handle, AudioGraph graph) : base(handle, graph) { }

    public SVFilterNode(AudioGraph graph, string name = null)
        : base(graph.CreateNode("SVFilter", name ?? "").Handle, graph) { }

    public float Cutoff
    {
        get => GetParameter("cutoff");
        set => SetParameter("cutoff", value);
    }

    public float Resonance
    {
        get => GetParameter("resonance");
        set => SetParameter("resonance", value);
    }

    public SVFMode Mode
    {
        get => (SVFMode)(int)GetParameter("mode");
        set => SetParameter("mode", (float)(int)value);
    }
}
