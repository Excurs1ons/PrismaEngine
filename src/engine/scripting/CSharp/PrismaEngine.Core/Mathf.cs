using System;
using System.Runtime.CompilerServices;

namespace PrismaEngine;

/// <summary>
/// 数学工具库。
/// Math utility library.
/// </summary>
public static class Mathf
{
    /// <summary>圆周率。 PI constant.</summary>
    public const float PI = MathF.PI;
    /// <summary>正无穷大。 Positive infinity.</summary>
    public const float Infinity = float.PositiveInfinity;
    /// <summary>负无穷大。 Negative infinity.</summary>
    public const float NegativeInfinity = float.NegativeInfinity;
    /// <summary>度到弧度的转换系数。 Degrees-to-radians conversion constant.</summary>
    public const float Deg2Rad = PI / 180.0f;
    /// <summary>弧度到度的转换系数。 Radians-to-degrees conversion constant.</summary>
    public const float Rad2Deg = 180.0f / PI;
    /// <summary>一个很小的浮点数值。 A tiny floating point value.</summary>
    public const float Epsilon = 1.401298E-45f;

    /// <summary>计算正弦值。 Returns the sine of f.</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static float Sin(float f) => MathF.Sin(f);
    /// <summary>计算余弦值。 Returns the cosine of f.</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static float Cos(float f) => MathF.Cos(f);
    /// <summary>计算正切值。 Returns the tangent of f.</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static float Tan(float f) => MathF.Tan(f);
    /// <summary>计算反正弦值。 Returns the arcsine of f.</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static float Asin(float f) => MathF.Asin(f);
    /// <summary>计算反余弦值。 Returns the arccosine of f.</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static float Acos(float f) => MathF.Acos(f);
    /// <summary>计算反正切值。 Returns the arctangent of f.</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static float Atan(float f) => MathF.Atan(f);
    /// <summary>计算反正切值 (y/x)。 Returns the angle in radians whose Tan is y/x.</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static float Atan2(float y, float x) => MathF.Atan2(y, x);
    /// <summary>计算平方根。 Returns the square root of f.</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static float Sqrt(float f) => MathF.Sqrt(f);
    /// <summary>计算绝对值。 Returns the absolute value of f.</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static float Abs(float f) => MathF.Abs(f);
    /// <summary>计算幂。 Returns f raised to power p.</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static float Pow(float f, float p) => MathF.Pow(f, p);
    /// <summary>计算 e 的幂。 Returns e raised to the specified power.</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static float Exp(float f) => MathF.Exp(f);
    /// <summary>计算自然对数。 Returns the natural logarithm of f.</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static float Log(float f) => MathF.Log(f);
    /// <summary>计算以 10 为底的对数。 Returns the base-10 logarithm of f.</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static float Log10(float f) => MathF.Log10(f);
    /// <summary>向上取整。 Returns the smallest integer greater than or equal to f.</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static float Ceil(float f) => MathF.Ceiling(f);
    /// <summary>向下取整。 Returns the largest integer smaller than or equal to f.</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static float Floor(float f) => MathF.Floor(f);
    /// <summary>四舍五入。 Returns f rounded to the nearest integer.</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static float Round(float f) => MathF.Round(f);
    /// <summary>向上取整为整数。 Returns the smallest integer greater than or equal to f.</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static int CeilToInt(float f) => (int)MathF.Ceiling(f);
    /// <summary>向下取整为整数。 Returns the largest integer smaller than or equal to f.</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static int FloorToInt(float f) => (int)MathF.Floor(f);
    /// <summary>四舍五入为整数。 Returns f rounded to the nearest integer.</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static int RoundToInt(float f) => (int)MathF.Round(f);
    /// <summary>返回数字的符号。 Returns the sign of f.</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static float Sign(float f) => f >= 0.0f ? 1.0f : -1.0f;
    
    /// <summary>
    /// 将值限制在 min 和 max 之间。
    /// Clamps a value between min and max.
    /// </summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static float Clamp(float value, float min, float max)
    {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    }

    /// <summary>
    /// 将整数值限制在 min 和 max 之间。
    /// Clamps an integer value between min and max.
    /// </summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static int Clamp(int value, int min, int max)
    {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    }

    /// <summary>
    /// 将值限制在 0 和 1 之间。
    /// Clamps value between 0 and 1 and returns value.
    /// </summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static float Clamp01(float value)
    {
        if (value < 0.0f) return 0.0f;
        if (value > 1.0f) return 1.0f;
        return value;
    }

    /// <summary>
    /// 在 a 和 b 之间进行线性插值。
    /// Linearly interpolates between a and b by t.
    /// </summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static float Lerp(float a, float b, float t)
    {
        return a + (b - a) * Clamp01(t);
    }

    /// <summary>
    /// 在 a 和 b 之间进行线性插值（不限制 t 的范围）。
    /// Linearly interpolates between a and b by t, without clamping t.
    /// </summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static float LerpUnclamped(float a, float b, float t)
    {
        return a + (b - a) * t;
    }

    /// <summary>返回两个值中的最大值。 Returns the largest of two or more values.</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static float Max(float a, float b) => MathF.Max(a, b);
    /// <summary>返回两个值中的最小值。 Returns the smallest of two or more values.</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static float Min(float a, float b) => MathF.Min(a, b);
    /// <summary>返回两个整数中的最大值。 Returns the largest of two or more values.</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static int Max(int a, int b) => Math.Max(a, b);
    /// <summary>返回两个整数中的最小值。 Returns the smallest of two or more values.</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static int Min(int a, int b) => Math.Min(a, b);

    /// <summary>
    /// 循环值 t，使其永远不会大于 length 且永远不会小于 0。
    /// Loops the value t, so that it is never larger than length and never smaller than 0.
    /// </summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static float Repeat(float t, float length)
    {
        return Clamp(t - MathF.Floor(t / length) * length, 0.0f, length);
    }

    /// <summary>
    /// 乒乓循环，返回一个在 0 和 length 之间来回移动的值。
    /// Pingpongs the value t, so that it is never larger than length and never smaller than 0.
    /// </summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static float PingPong(float t, float length)
    {
        t = Repeat(t, length * 2.0f);
        return length - MathF.Abs(t - length);
    }

    /// <summary>
    /// 计算两个值之间的插值参数 t。
    /// Calculates the linear parameter t that produces the interpolant value within the range [a, b].
    /// </summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static float InverseLerp(float a, float b, float value)
    {
        if (a != b) return Clamp01((value - a) / (b - a));
        return 0.0f;
    }
}
