using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using NumericsVector4 = System.Numerics.Vector4;

namespace PrismaEngine;

/// <summary>
/// 4D 向量。
/// 4D Vector.
/// </summary>
[StructLayout(LayoutKind.Explicit)]
public struct Vector4 : IEquatable<Vector4>, ISpanFormattable
{
    /// <summary> X 分量。 X component. </summary>
    [FieldOffset(0)] public float X;
    /// <summary> Y 分量。 Y component. </summary>
    [FieldOffset(4)] public float Y;
    /// <summary> Z 分量。 Z component. </summary>
    [FieldOffset(8)] public float Z;
    /// <summary> W 分量。 W component. </summary>
    [FieldOffset(12)] public float W;

    /// <summary> 构造一个新的 4D 向量。 Constructs a new 4D vector. </summary>
    public Vector4(float x, float y, float z, float w) { X = x; Y = y; Z = z; W = w; }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static ref readonly NumericsVector4 AsNumerics(in Vector4 v) => ref Unsafe.As<Vector4, NumericsVector4>(ref Unsafe.AsRef(in v));

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static ref Vector4 FromNumerics(ref NumericsVector4 v) => ref Unsafe.As<NumericsVector4, Vector4>(ref v);

    /// <summary> 零向量 (0, 0, 0, 0)。 Zero vector (0, 0, 0, 0). </summary>
    public static readonly Vector4 Zero = new(0, 0, 0, 0);
    /// <summary> 单位向量 (1, 1, 1, 1)。 One vector (1, 1, 1, 1). </summary>
    public static readonly Vector4 One = new(1, 1, 1, 1);

    /// <summary> XY 分量。 XY components as Vector2. </summary>
    public Vector2 XY => new Vector2(X, Y);
    /// <summary> ZW 分量。 ZW components as Vector2. </summary>
    public Vector2 ZW => new Vector2(Z, W);

    /// <summary> 检查相等性。 Checks for equality. </summary>
    public bool Equals(Vector4 other) => this == other;
    /// <summary> 检查相等性。 Checks for equality. </summary>
    public override bool Equals(object? obj) => obj is Vector4 other && Equals(other);
    /// <summary> 获取哈希码。 Gets the hash code. </summary>
    public override int GetHashCode() => HashCode.Combine(X, Y, Z, W);

    /// <summary> 相等。 Equality. </summary>
    public static bool operator ==(Vector4 a, Vector4 b) => a.X == b.X && a.Y == b.Y && a.Z == b.Z && a.W == b.W;
    /// <summary> 不相等。 Inequality. </summary>
    public static bool operator !=(Vector4 a, Vector4 b) => !(a == b);

    /// <summary> 加法。 Addition. </summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Vector4 operator +(Vector4 a, Vector4 b) { var v = AsNumerics(a) + AsNumerics(b); return FromNumerics(ref v); }
    /// <summary> 减法。 Subtraction. </summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Vector4 operator -(Vector4 a, Vector4 b) { var v = AsNumerics(a) - AsNumerics(b); return FromNumerics(ref v); }
    /// <summary> 乘法。 Multiplication. </summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Vector4 operator *(Vector4 a, float s) { var v = AsNumerics(a) * s; return FromNumerics(ref v); }
    /// <summary> 乘法。 Multiplication. </summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Vector4 operator *(float s, Vector4 a) { var v = AsNumerics(a) * s; return FromNumerics(ref v); }
    /// <summary> 除法。 Division. </summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Vector4 operator /(Vector4 a, float d) { var v = AsNumerics(a) / d; return FromNumerics(ref v); }

    /// <summary> 转换为字符串。 Converts to string. </summary>
    public override string ToString() => $"({X:F2}, {Y:F2}, {Z:F2}, {W:F2})";

    /// <summary> 零分配格式化支持。 Zero-allocation formatting support. </summary>
    public bool TryFormat(Span<char> destination, out int charsWritten, ReadOnlySpan<char> format = default, IFormatProvider? provider = null)
    {
        return destination.TryWrite(provider, $"({X:F2}, {Y:F2}, {Z:F2}, {W:F2})", out charsWritten);
    }

    public string ToString(string? format, IFormatProvider? formatProvider) => ToString();
}