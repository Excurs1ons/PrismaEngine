using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using NumericsVector2 = System.Numerics.Vector2;

namespace PrismaEngine;

/// <summary>
/// 2D 向量。
/// 2D Vector.
/// </summary>
[StructLayout(LayoutKind.Explicit)]
public struct Vector2 : IEquatable<Vector2>, ISpanFormattable
{
    /// <summary> X 分量。 X component. </summary>
    [FieldOffset(0)] public float X;
    /// <summary> Y 分量。 Y component. </summary>
    [FieldOffset(4)] public float Y;

    /// <summary> 构造一个新的 2D 向量。 Constructs a new 2D vector. </summary>
    public Vector2(float x, float y) { X = x; Y = y; }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static ref readonly NumericsVector2 AsNumerics(in Vector2 v) => ref Unsafe.As<Vector2, NumericsVector2>(ref Unsafe.AsRef(in v));

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static ref Vector2 FromNumerics(ref NumericsVector2 v) => ref Unsafe.As<NumericsVector2, Vector2>(ref v);

    /// <summary> 零向量 (0, 0)。 Zero vector (0, 0). </summary>
    public static readonly Vector2 Zero = new(0, 0);
    /// <summary> 单位向量 (1, 1)。 One vector (1, 1). </summary>
    public static readonly Vector2 One = new(1, 1);

    /// <summary> 长度的平方。 Squared magnitude. </summary>
    public float SqrMagnitude => AsNumerics(this).LengthSquared();
    /// <summary> 长度。 Magnitude. </summary>
    public float Magnitude => AsNumerics(this).Length();
    /// <summary> 归一化后的向量。 Normalized vector. </summary>
    public Vector2 Normalized { get { var v = NumericsVector2.Normalize(AsNumerics(this)); return FromNumerics(ref v); } }

    /// <summary> 检查相等性。 Checks for equality. </summary>
    public bool Equals(Vector2 other) => this == other;
    /// <summary> 检查相等性。 Checks for equality. </summary>
    public override bool Equals(object? obj) => obj is Vector2 other && Equals(other);
    /// <summary> 获取哈希码。 Gets the hash code. </summary>
    public override int GetHashCode() => HashCode.Combine(X, Y);

    /// <summary> 相等。 Equality. </summary>
    public static bool operator ==(Vector2 a, Vector2 b) => a.X == b.X && a.Y == b.Y;
    /// <summary> 不相等。 Inequality. </summary>
    public static bool operator !=(Vector2 a, Vector2 b) => !(a == b);

    /// <summary> 加法。 Addition. </summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Vector2 operator +(Vector2 a, Vector2 b) { var v = AsNumerics(a) + AsNumerics(b); return FromNumerics(ref v); }
    /// <summary> 减法。 Subtraction. </summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Vector2 operator -(Vector2 a, Vector2 b) { var v = AsNumerics(a) - AsNumerics(b); return FromNumerics(ref v); }
    /// <summary> 取反。 Negation. </summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Vector2 operator -(Vector2 a) { var v = -AsNumerics(a); return FromNumerics(ref v); }
    /// <summary> 乘法。 Multiplication. </summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Vector2 operator *(Vector2 a, float s) { var v = AsNumerics(a) * s; return FromNumerics(ref v); }
    /// <summary> 乘法。 Multiplication. </summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Vector2 operator *(float s, Vector2 a) { var v = AsNumerics(a) * s; return FromNumerics(ref v); }
    /// <summary> 除法。 Division. </summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Vector2 operator /(Vector2 a, float d) { var v = AsNumerics(a) / d; return FromNumerics(ref v); }

    /// <summary> 转换为字符串。 Converts to string. </summary>
    public override string ToString() => $"({X:F2}, {Y:F2})";

    /// <summary> 零分配格式化支持。 Zero-allocation formatting support. </summary>
    public bool TryFormat(Span<char> destination, out int charsWritten, ReadOnlySpan<char> format = default, IFormatProvider? provider = null)
    {
        return destination.TryWrite(provider, $"({X:F2}, {Y:F2})", out charsWritten);
    }

    public string ToString(string? format, IFormatProvider? formatProvider) => ToString();
}
