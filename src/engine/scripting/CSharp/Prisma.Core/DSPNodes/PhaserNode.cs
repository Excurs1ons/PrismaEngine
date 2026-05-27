using System;

namespace Prisma;

public class PhaserNode : DSPNode
{
    internal PhaserNode(ulong handle, AudioGraph graph) : base(handle, graph) { }

    public PhaserNode(AudioGraph graph, string name = null)
        : base(graph.CreateNode("Phaser", name ?? "").Handle, graph) { }

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

    public float Stages
    {
        get => GetParameter("stages");
        set => SetParameter("stages", value);
    }

    public float Feedback
    {
        get => GetParameter("feedback");
        set => SetParameter("feedback", value);
    }

    public float Mix
    {
        get => GetParameter("mix");
        set => SetParameter("mix", value);
    }

    public float CenterFreq
    {
        get => GetParameter("centerFreq");
        set => SetParameter("centerFreq", value);
    }
}
