using System;
using System.Threading;

namespace PrismaEngine;

/// <summary>
/// 随机数生成工具类。
/// Utility class for random number generation.
/// </summary>
public static class Random
{
    // Cherno Optimization: 使用 ThreadLocal 确保多线程环境下的随机数生成器安全且高效。
    // 避免了共享单个 System.Random 导致的内部状态崩溃。
    private static readonly ThreadLocal<System.Random> _localRng = new(() => new System.Random(Guid.NewGuid().GetHashCode()));

    private static System.Random Instance => _localRng.Value!;

    /// <summary>
    /// 返回一个在 [0, 1] 范围内的随机浮点数。
    /// Returns a random float between 0.0 and 1.0 (inclusive).
    /// </summary>
    public static float value => (float)Instance.NextDouble();

    /// <summary>
    /// 返回一个在 [min, max] 范围内的随机浮点数。
    /// Returns a random float between min and max (inclusive).
    /// </summary>
    public static float Range(float min, float max)
        => (float)(Instance.NextDouble() * (max - min) + min);

    /// <summary>
    /// 返回一个在 [min, max) 范围内的随机整数。
    /// Returns a random integer between min and max (exclusive of max).
    /// </summary>
    public static int Range(int min, int max)
        => Instance.Next(min, max);

    /// <summary>
    /// 返回半径为 1 的圆内的一个随机点。
    /// Returns a random point inside a circle with radius 1.
    /// </summary>
    public static Vector2 insideUnitCircle
    {
        get
        {
            float angle = value * Mathf.PI * 2.0f;
            float radius = Mathf.Sqrt(value);
            return new Vector2(Mathf.Cos(angle) * radius, Mathf.Sin(angle) * radius);
        }
    }
}
