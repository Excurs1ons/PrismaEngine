using System;

namespace Prisma;

public class GraphicEQNode : DSPNode
{
    internal GraphicEQNode(ulong handle, AudioGraph graph) : base(handle, graph) { }

    public GraphicEQNode(AudioGraph graph, string name = null)
        : base(graph.CreateNode("GraphicEQ", name ?? "").Handle, graph) { }

    public float this[int index]
    {
        get
        {
            if (index < 0 || index > 9)
                throw new ArgumentOutOfRangeException(nameof(index), "Band index must be 0-9");
            return GetParameter($"band{index}");
        }
        set
        {
            if (index < 0 || index > 9)
                throw new ArgumentOutOfRangeException(nameof(index), "Band index must be 0-9");
            SetParameter($"band{index}", value);
        }
    }

    public float Band0
    {
        get => GetParameter("band0");
        set => SetParameter("band0", value);
    }

    public float Band1
    {
        get => GetParameter("band1");
        set => SetParameter("band1", value);
    }

    public float Band2
    {
        get => GetParameter("band2");
        set => SetParameter("band2", value);
    }

    public float Band3
    {
        get => GetParameter("band3");
        set => SetParameter("band3", value);
    }

    public float Band4
    {
        get => GetParameter("band4");
        set => SetParameter("band4", value);
    }

    public float Band5
    {
        get => GetParameter("band5");
        set => SetParameter("band5", value);
    }

    public float Band6
    {
        get => GetParameter("band6");
        set => SetParameter("band6", value);
    }

    public float Band7
    {
        get => GetParameter("band7");
        set => SetParameter("band7", value);
    }

    public float Band8
    {
        get => GetParameter("band8");
        set => SetParameter("band8", value);
    }

    public float Band9
    {
        get => GetParameter("band9");
        set => SetParameter("band9", value);
    }
}
