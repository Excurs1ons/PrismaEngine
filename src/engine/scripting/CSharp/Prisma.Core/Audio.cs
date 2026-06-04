using System;
using System.Runtime.InteropServices;

namespace Prisma;

public static unsafe class Audio
{
    public static bool IsInitialized => Interop.API.IsAudioInitialized();

    public static float MasterVolume
    {
        get => Interop.API.AudioGetMasterVolume();
        set => Interop.API.AudioSetMasterVolume(value);
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
            return Interop.API.AudioPlayClip(p, &desc);
        }
    }

    public static void Stop(uint voiceId) => Interop.API.AudioStop(voiceId);
    public static void StopAll() => Interop.API.AudioStopAll();
    public static void SetVolume(uint voiceId, float volume) => Interop.API.AudioSetVolume(voiceId, volume);
    public static bool IsPlaying(uint voiceId) => Interop.API.AudioIsPlaying(voiceId);

    // ===== High-Level Audio API (BGM/SFX) =====

    public static void PlaySFX(string clipPath, float volume = 1.0f, float pitch = 1.0f)
    {
        byte[] bytes = System.Text.Encoding.UTF8.GetBytes(clipPath + "\0");
        fixed (byte* p = bytes) {
            Interop.API.AudioPlaySFX(p, volume, pitch);
        }
    }

    public static void PlayBGM(string clipPath, float volume = 1.0f, bool loop = true)
    {
        byte[] bytes = System.Text.Encoding.UTF8.GetBytes(clipPath + "\0");
        fixed (byte* p = bytes) {
            Interop.API.AudioPlayBGM(p, volume, loop);
        }
    }

    public static void StopBGM(float fadeOutMs = 0) => Interop.API.AudioStopBGM(fadeOutMs);

    public static float BGMVolume
    {
        set => Interop.API.AudioSetBGMVolume(value);
    }

    public static float SFXVolume
    {
        set => Interop.API.AudioSetSFXVolume(value);
    }

    public static void SetListenerPosition(float x, float y) => Interop.API.AudioSetListenerPosition(x, y);
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
