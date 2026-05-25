using System;

namespace Prisma;

public class DelayNode : DSPNode
{
    internal DelayNode(ulong handle, AudioGraph graph) : base(handle, graph) { }

    public DelayNode(AudioGraph graph, string name = null)
        : base(graph.CreateNode("Delay", name ?? "").Handle, graph) { }

    public float DelayTime
    {
        get => GetParameter("delayTime");
        set => SetParameter("delayTime", value);
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

    public float Lowpass
    {
        get => GetParameter("lowpass");
        set => SetParameter("lowpass", value);
    }

    public float StereoSpread
    {
        get => GetParameter("stereoSpread");
        set => SetParameter("stereoSpread", value);
    }
}
