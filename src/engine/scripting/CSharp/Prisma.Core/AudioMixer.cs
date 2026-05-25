using System;
using System.Collections.Generic;

namespace Prisma;

public class AudioMixer : IDisposable
{
    private readonly AudioGraph _graph;
    private readonly MasterBusNode _masterBus;
    private readonly List<GroupBusNode> _groupBuses = new();
    private readonly List<AuxBusNode> _auxBuses = new();
    private bool _disposed;

    public AudioMixer(AudioGraph graph, uint sampleRate = 48000, uint framesPerBlock = 256)
    {
        _graph = graph;
        _masterBus = new MasterBusNode(graph, "Master");
    }

    public MasterBusNode MasterBus => _masterBus;
    public IReadOnlyList<GroupBusNode> GroupBuses => _groupBuses;
    public IReadOnlyList<AuxBusNode> AuxBuses => _auxBuses;

    public GroupBusNode CreateGroupBus(string name = null)
    {
        var bus = new GroupBusNode(_graph, name ?? $"Group{_groupBuses.Count}");
        // Auto-connect group bus to master bus
        _graph.Connect(bus, "Output", _masterBus, "Input");
        _groupBuses.Add(bus);
        return bus;
    }

    public AuxBusNode CreateAuxBus(string name = null)
    {
        var bus = new AuxBusNode(_graph, name ?? $"Aux{_auxBuses.Count}");
        // Auto-connect aux bus to master
        _graph.Connect(bus, "Output", _masterBus, "Input");
        _auxBuses.Add(bus);
        return bus;
    }

    // Route a source DSPNode to a GroupBus
    public void RouteToGroup(DSPNode source, GroupBusNode targetGroup)
    {
        _graph.Connect(source, "Output", targetGroup, "Input");
    }

    // Route a source DSPNode directly to Master
    public void RouteToMaster(DSPNode source)
    {
        _graph.Connect(source, "Output", _masterBus, "Input");
    }

    // Route a source DSPNode to an AuxBus (for send effects)
    public void RouteToAux(DSPNode source, AuxBusNode auxBus)
    {
        _graph.Connect(source, "Output", auxBus, "Input");
    }

    public void Dispose()
    {
        if (_disposed) return;
        _disposed = true;
        _masterBus.Dispose();
        foreach (var bus in _groupBuses) bus.Dispose();
        foreach (var bus in _auxBuses) bus.Dispose();
        _groupBuses.Clear();
        _auxBuses.Clear();
    }
}
