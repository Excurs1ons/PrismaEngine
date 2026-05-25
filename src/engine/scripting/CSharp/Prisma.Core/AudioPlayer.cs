using System;
using System.Collections.Generic;

namespace Prisma;

public class AudioPlayer : IDisposable
{
    private readonly List<PlaybackVoice> _activeVoices = new();
    private bool _disposed;

    public IReadOnlyList<PlaybackVoice> ActiveVoices => _activeVoices;

    public PlaybackVoice Play(string clipPath, float volume = 1.0f, float pitch = 1.0f, bool loop = false)
    {
        if (_disposed) throw new ObjectDisposedException(nameof(AudioPlayer));
        uint voiceId = Audio.PlayClip(clipPath, volume, pitch, loop);
        // voiceId == 0 means failed
        var voice = new PlaybackVoice(voiceId, clipPath);
        if (voiceId != 0) _activeVoices.Add(voice);
        return voice;
    }

    public void Stop(PlaybackVoice voice)
    {
        if (voice == null) return;
        Audio.Stop(voice.Id);
        _activeVoices.Remove(voice);
    }

    public void StopAll()
    {
        Audio.StopAll();
        _activeVoices.Clear();
    }

    public void SetVolume(PlaybackVoice voice, float volume)
    {
        if (voice == null) return;
        Audio.SetVolume(voice.Id, volume);
    }

    public void Dispose()
    {
        if (_disposed) return;
        _disposed = true;
        StopAll();
    }
}
