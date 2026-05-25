using System;

namespace Prisma;

public class AuxBusNode : DSPNode
{
    internal AuxBusNode(ulong handle, AudioGraph graph) : base(handle, graph) { }

    public AuxBusNode(AudioGraph graph, string name = null)
        : base(graph.CreateNode("AuxBus", name ?? "").Handle, graph) { }

    public float ReturnLevel
    {
        get => GetParameter("returnLevel");
        set => SetParameter("returnLevel", value);
    }

    public float PreFader
    {
        get => GetParameter("preFader");
        set => SetParameter("preFader", value);
    }
}
