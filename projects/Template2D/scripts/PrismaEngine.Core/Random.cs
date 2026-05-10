namespace PrismaEngine;

/// <summary>
/// 随机数辅助。
/// </summary>
public static class Random
{
    private static System.Random _rng = new();

    /// <summary>返回 [min, max) 范围内的随机浮点数。</summary>
    public static float Range(float min, float max)
        => (float)(_rng.NextDouble() * (max - min) + min);

    /// <summary>返回 [min, max) 范围内的随机整数。</summary>
    public static int Range(int min, int max)
        => _rng.Next(min, max);
}
