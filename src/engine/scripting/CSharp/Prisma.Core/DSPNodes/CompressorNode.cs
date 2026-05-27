using System;

namespace Prisma;

public class CompressorNode : DSPNode
{
    internal CompressorNode(ulong handle, AudioGraph graph) : base(handle, graph) { }

    public CompressorNode(AudioGraph graph, string name = null)
        : base(graph.CreateNode("Compressor", name ?? "").Handle, graph) { }

    public float Threshold
    {
        get => GetParameter("threshold");
        set => SetParameter("threshold", value);
    }

    public float Ratio
    {
        get => GetParameter("ratio");
        set => SetParameter("ratio", value);
    }

    public float Attack
    {
        get => GetParameter("attack");
        set => SetParameter("attack", value);
    }

    public float Release
    {
        get => GetParameter("release");
        set => SetParameter("release", value);
    }

    public float Makeup
    {
        get => GetParameter("makeup");
        set => SetParameter("makeup", value);
    }

    public float Knee
    {
        get => GetParameter("knee");
        set => SetParameter("knee", value);
    }

    public float Mix
    {
        get => GetParameter("mix");
        set => SetParameter("mix", value);
    }
}
