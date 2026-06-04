using System;
using Prisma;

namespace GameScripts;

public static class AudioManager
{
    public static uint RegisterZone(float x1, float y1, float x2, float y2, string bgmPath, float volume = 1f, bool loop = true, float fadeMs = 500f)
    {
        unsafe
        {
            var pathBytes = Interop.StringToUtf8(bgmPath);
            fixed (byte* p = pathBytes)
            {
                return Interop.API.AudioZoneRegister(x1, y1, x2, y2, p, volume, loop, fadeMs);
            }
        }
    }

    public static void UnregisterZone(uint id)
    {
        unsafe { Interop.API.AudioZoneUnregister(id); }
    }

    public static void SetPlayerPos(float x, float y)
    {
        unsafe { Interop.API.AudioZoneSetPlayerPos(x, y); }
    }

    public static void PlaySFX(string clipPath, float volume = 1f)
    {
        unsafe
        {
            var pathBytes = Interop.StringToUtf8(clipPath);
            fixed (byte* p = pathBytes)
            {
                Interop.API.AudioZonePlaySFX(p, volume);
            }
        }
    }
}
