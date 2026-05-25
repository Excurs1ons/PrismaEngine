using System;

namespace Prisma;

public class SynthVoiceManager : DSPNode
{
    internal SynthVoiceManager(ulong handle, AudioGraph graph) : base(handle, graph) { }

    public SynthVoiceManager(AudioGraph graph, string name = null)
        : base(graph.CreateNode("SynthVoiceManager", name ?? "").Handle, graph) { }

    public float MaxVoices
    {
        get => GetParameter("maxVoices");
        set => SetParameter("maxVoices", value);
    }

    public float Volume
    {
        get => GetParameter("volume");
        set => SetParameter("volume", value);
    }

    public float Unison
    {
        get => GetParameter("unison");
        set => SetParameter("unison", value);
    }

    public float Detune
    {
        get => GetParameter("detune");
        set => SetParameter("detune", value);
    }
}
