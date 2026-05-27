using System;
using System.Runtime.InteropServices;

namespace Prisma;

public class LevelMeterNode : DSPNode
{
    internal LevelMeterNode(ulong handle, AudioGraph graph) : base(handle, graph) { }

    public LevelMeterNode(AudioGraph graph, string name = null)
        : base(graph.CreateNode("LevelMeter", name ?? "").Handle, graph) { }

    [StructLayout(LayoutKind.Sequential)]
    public struct ChannelLevel
    {
        public float Peak;
        public float Rms;
        public float PeakDb;
        public float RmsDb;
    }

    public ChannelLevel GetChannelLevel(uint channel)
    {
        unsafe
        {
            ChannelLevel level;
            bool ok = Interop.API.AudioGetLevelMeterData(
                Handle, channel, &level.Peak, &level.Rms, &level.PeakDb, &level.RmsDb);
            if (!ok) return default;
            return level;
        }
    }
}
