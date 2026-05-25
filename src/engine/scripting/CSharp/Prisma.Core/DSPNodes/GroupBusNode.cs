using System;

namespace Prisma;

public class GroupBusNode : DSPNode
{
    internal GroupBusNode(ulong handle, AudioGraph graph) : base(handle, graph) { }

    public GroupBusNode(AudioGraph graph, string name = null)
        : base(graph.CreateNode("GroupBus", name ?? "").Handle, graph) { }

    public float Volume
    {
        get => GetParameter("volume");
        set => SetParameter("volume", value);
    }

    public float Pan
    {
        get => GetParameter("pan");
        set => SetParameter("pan", value);
    }

    public bool Mute
    {
        get => GetParameter("mute") > 0.5f;
        set => SetParameter("mute", value ? 1.0f : 0.0f);
    }

    public float AuxSendLevel
    {
        get => GetParameter("auxSendLevel");
        set => SetParameter("auxSendLevel", value);
    }
}
