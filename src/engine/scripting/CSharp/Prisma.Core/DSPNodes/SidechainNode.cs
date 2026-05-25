using System;

namespace Prisma;

public class SidechainNode : DSPNode
{
    internal SidechainNode(ulong handle, AudioGraph graph) : base(handle, graph) { }

    public SidechainNode(AudioGraph graph, string name = null)
        : base(graph.CreateNode("Sidechain", name ?? "").Handle, graph) { }

    public float Ratio
    {
        get => GetParameter("ratio");
        set => SetParameter("ratio", value);
    }

    public float Attack
    {
        get => GetParameter("attack");
        set => SetParameter("attack", value);
    }

    public float Release
    {
        get => GetParameter("release");
        set => SetParameter("release", value);
    }
}
