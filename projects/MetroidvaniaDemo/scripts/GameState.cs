using System;
using Prisma;

namespace GameScripts;

/// <summary>
/// Shared game state accessible by all scripts.
/// </summary>
public static class GameState
{
    public static bool HasDash { get; set; } = false;
    public static Node PlayerNode { get; set; }
    public static uint TilemapHandle { get; set; } = 0;
    public static float[] SolidTileData { get; set; } = Array.Empty<float>();
    public static int SolidTileCount { get; set; } = 0;

    // Camera bounds (center of viewport)
    public static float CameraMinX { get; set; } = 128f;
    public static float CameraMinY { get; set; } = 112f;
    public static float CameraMaxX { get; set; } = 1792f;
    public static float CameraMaxY { get; set; } = 368f;
}
