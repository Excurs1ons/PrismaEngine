using System;

namespace Prisma;

public class MasterBusNode : DSPNode
{
    internal MasterBusNode(ulong handle, AudioGraph graph) : base(handle, graph) { }

    public MasterBusNode(AudioGraph graph, string name = null)
        : base(graph.CreateNode("MasterBus", name ?? "").Handle, graph) { }

    public float Volume
    {
        get => GetParameter("volume");
        set => SetParameter("volume", value);
    }

    public bool Mute
    {
        get => GetParameter("mute") > 0.5f;
        set => SetParameter("mute", value ? 1.0f : 0.0f);
    }

    public bool Solo
    {
        get => GetParameter("solo") > 0.5f;
        set => SetParameter("solo", value ? 1.0f : 0.0f);
    }
}
