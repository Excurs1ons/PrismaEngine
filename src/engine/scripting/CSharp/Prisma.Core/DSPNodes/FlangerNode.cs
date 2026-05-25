using System;

namespace Prisma;

public class FlangerNode : DSPNode
{
    internal FlangerNode(ulong handle, AudioGraph graph) : base(handle, graph) { }

    public FlangerNode(AudioGraph graph, string name = null)
        : base(graph.CreateNode("Flanger", name ?? "").Handle, graph) { }

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
}
