using System;
using System.Runtime.InteropServices;

namespace Prisma;

public static class Audio
{
    public static bool IsInitialized => API.IsAudioInitialized();

    public static float MasterVolume
    {
        get => API.AudioGetMasterVolume();
        set => API.AudioSetMasterVolume(value);
    }

    public static uint PlayClip(string path, float volume = 1.0f, float pitch = 1.0f, bool loop = false)
    {
        fixed (byte* p = System.Text.Encoding.UTF8.GetBytes(path + "\0"))
        {
            var desc = new AudioPlayDesc
            {
                volume = volume,
                pitch = pitch,
                loop = loop ? 1 : 0
            };
            return API.AudioPlayClip(p, &desc);
        }
    }

    public static void Stop(uint voiceId) => API.AudioStop(voiceId);
    public static void StopAll() => API.AudioStopAll();
    public static void SetVolume(uint voiceId, float volume) => API.AudioSetVolume(voiceId, volume);
    public static bool IsPlaying(uint voiceId) => API.AudioIsPlaying(voiceId);
}

[StructLayout(LayoutKind.Sequential)]
internal struct AudioPlayDesc
{
    public float volume;
    public float pitch;
    public int loop;
    public float startTime;
    public float endTime;
    public int is3D;
    public float posX;
    public float posY;
    public float posZ;
}
