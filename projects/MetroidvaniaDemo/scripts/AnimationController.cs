using System;
using Prisma;

namespace GameScripts;

/// <summary>
/// Programmatic sprite animation via Interop.RenderData color/size manipulation.
/// All methods are pure functions — they modify RenderData arrays directly.
/// </summary>
public static class AnimationController
{
    /// <summary>
    /// Pulse between two colors at a given speed.
    /// </summary>
    public static void PulseColor(uint idx, float time, float r1, float g1, float b1, float r2, float g2, float b2, float speed)
    {
        float t = (Mathf.Sin(time * speed) + 1f) * 0.5f; // 0..1 sine wave
        unsafe
        {
            Interop.RenderData->ColorR[idx] = r1 + (r2 - r1) * t;
            Interop.RenderData->ColorG[idx] = g1 + (g2 - g1) * t;
            Interop.RenderData->ColorB[idx] = b1 + (b2 - b1) * t;
        }
    }

    /// <summary>
    /// Bounce size (width/height) within a range at a given frequency.
    /// </summary>
    public static void BounceSize(uint idx, float time, float baseW, float baseH, float amplitude, float frequency)
    {
        float t = Mathf.Abs(Mathf.Sin(time * frequency));
        float scale = 1f + amplitude * t;
        unsafe
        {
            Interop.RenderData->SizeW[idx] = baseW * scale;
            Interop.RenderData->SizeH[idx] = baseH * scale;
        }
    }

    /// <summary>
    /// Float an item up and down with sine wave (for pickup items).
    /// Returns the Y offset for caller to apply to node.Y.
    /// </summary>
    public static float FloatOffset(float time, float amplitude, float speed)
    {
        return Mathf.Sin(time * speed) * amplitude;
    }

    /// <summary>
    /// Flash a color briefly (for damage feedback).
    /// Returns true during flash window.
    /// </summary>
    public static bool DamageFlash(float invincibleTimer, float flashInterval)
    {
        if (invincibleTimer <= 0f) return false;
        return (Mathf.Floor(invincibleTimer / flashInterval) % 2) < 1;
    }
}
