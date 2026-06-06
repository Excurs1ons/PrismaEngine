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

    public static int CurrentHP { get; set; } = 5;
    public static int MaxHP { get; set; } = 5;
    public static bool HasDoubleJump { get; set; } = false;
    public static Vector2 CheckpointPos { get; set; } = new Vector2(64, 408);
    public static float InvincibleTimer { get; set; } = 0f;
}
