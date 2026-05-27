using System;

namespace Prisma;

public class ConvolutionReverbNode : DSPNode
{
    internal ConvolutionReverbNode(ulong handle, AudioGraph graph) : base(handle, graph) { }

    public ConvolutionReverbNode(AudioGraph graph, string name = null)
        : base(graph.CreateNode("ConvolutionReverb", name ?? "").Handle, graph) { }

    public float Mix
    {
        get => GetParameter("mix");
        set => SetParameter("mix", value);
    }

    public float Gain
    {
        get => GetParameter("gain");
        set => SetParameter("gain", value);
    }
}
