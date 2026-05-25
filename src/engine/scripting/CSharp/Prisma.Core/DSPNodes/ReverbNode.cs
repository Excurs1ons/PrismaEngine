using System;

namespace Prisma;

public class ReverbNode : DSPNode
{
    internal ReverbNode(ulong handle, AudioGraph graph) : base(handle, graph) { }

    public ReverbNode(AudioGraph graph, string name = null)
        : base(graph.CreateNode("Reverb", name ?? "").Handle, graph) { }

    public float RoomSize
    {
        get => GetParameter("roomSize");
        set => SetParameter("roomSize", value);
    }

    public float Damping
    {
        get => GetParameter("damping");
        set => SetParameter("damping", value);
    }

    public float Width
    {
        get => GetParameter("width");
        set => SetParameter("width", value);
    }

    public float Mix
    {
        get => GetParameter("mix");
        set => SetParameter("mix", value);
    }

    public float PreDelay
    {
        get => GetParameter("preDelay");
        set => SetParameter("preDelay", value);
    }
}
