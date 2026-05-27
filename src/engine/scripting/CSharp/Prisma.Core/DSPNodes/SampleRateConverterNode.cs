using System;

namespace Prisma;

public class SampleRateConverterNode : DSPNode
{
    internal SampleRateConverterNode(ulong handle, AudioGraph graph) : base(handle, graph) { }

    public SampleRateConverterNode(AudioGraph graph, string name = null)
        : base(graph.CreateNode("SampleRateConverter", name ?? "").Handle, graph) { }

    public float Quality
    {
        get => GetParameter("quality");
        set => SetParameter("quality", value);
    }
}
