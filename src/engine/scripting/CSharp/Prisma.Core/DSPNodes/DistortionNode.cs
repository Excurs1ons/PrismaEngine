using System;

namespace Prisma;

public enum DistortionType
{
    HardClip,
    SoftClip,
    HalfWave,
    FullWave,
    BitCrush,
    Foldback
}

public class DistortionNode : DSPNode
{
    internal DistortionNode(ulong handle, AudioGraph graph) : base(handle, graph) { }

    public DistortionNode(AudioGraph graph, string name = null)
        : base(graph.CreateNode("Distortion", name ?? "").Handle, graph) { }

    public float Drive
    {
        get => GetParameter("drive");
        set => SetParameter("drive", value);
    }

    public float Tone
    {
        get => GetParameter("tone");
        set => SetParameter("tone", value);
    }

    public float Mix
    {
        get => GetParameter("mix");
        set => SetParameter("mix", value);
    }

    public DistortionType Type
    {
        get => (DistortionType)(int)GetParameter("type");
        set => SetParameter("type", (float)(int)value);
    }

    public float BitDepth
    {
        get => GetParameter("bitDepth");
        set => SetParameter("bitDepth", value);
    }
}
