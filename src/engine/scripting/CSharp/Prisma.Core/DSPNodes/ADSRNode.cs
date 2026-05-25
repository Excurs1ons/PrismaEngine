using System;

namespace Prisma;

public class ADSRNode : DSPNode
{
    internal ADSRNode(ulong handle, AudioGraph graph) : base(handle, graph) { }

    public ADSRNode(AudioGraph graph, string name = null)
        : base(graph.CreateNode("ADSR", name ?? "").Handle, graph) { }

    public float Attack
    {
        get => GetParameter("attack");
        set => SetParameter("attack", value);
    }

    public float Decay
    {
        get => GetParameter("decay");
        set => SetParameter("decay", value);
    }

    public float Sustain
    {
        get => GetParameter("sustain");
        set => SetParameter("sustain", value);
    }

    public float Release
    {
        get => GetParameter("release");
        set => SetParameter("release", value);
    }

    public float Gate
    {
        get => GetParameter("gate");
        set => SetParameter("gate", value);
    }
}
