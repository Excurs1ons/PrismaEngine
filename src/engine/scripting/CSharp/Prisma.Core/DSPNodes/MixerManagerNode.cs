using System;

namespace Prisma;

public class MixerManagerNode : DSPNode
{
    internal MixerManagerNode(ulong handle, AudioGraph graph) : base(handle, graph) { }

    public MixerManagerNode(AudioGraph graph, string name = null)
        : base(graph.CreateNode("MixerManager", name ?? "").Handle, graph) { }

    public float MasterVolume
    {
        get => GetParameter("masterVolume");
        set => SetParameter("masterVolume", value);
    }
}
