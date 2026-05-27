using System;

namespace Prisma;

public class ChorusNode : DSPNode
{
    internal ChorusNode(ulong handle, AudioGraph graph) : base(handle, graph) { }

    public ChorusNode(AudioGraph graph, string name = null)
        : base(graph.CreateNode("Chorus", name ?? "").Handle, graph) { }

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

    public float Delay
    {
        get => GetParameter("delay");
        set => SetParameter("delay", value);
    }

    public float Mix
    {
        get => GetParameter("mix");
        set => SetParameter("mix", value);
    }

    public float Voices
    {
        get => GetParameter("voices");
        set => SetParameter("voices", value);
    }
}
