using System;

namespace Prisma;

public class PlaybackVoice
{
    public uint Id { get; }
    public string ClipPath { get; }

    internal PlaybackVoice(uint id, string clipPath)
    {
        Id = id;
        ClipPath = clipPath;
    }

    public bool IsPlaying => Audio.IsPlaying(Id);

    public float Volume
    {
        get => 0f; // No getter from C++ side — kept for API symmetry
        set => Audio.SetVolume(Id, value);
    }

    public void Stop()
    {
        Audio.Stop(Id);
    }
}
