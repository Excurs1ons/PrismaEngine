using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using NumericsVector4 = System.Numerics.Vector4;

namespace Prisma;

/// <summary>
/// RGBA 颜色�?
/// RGBA Color.
/// </summary>
[StructLayout(LayoutKind.Explicit)]
public struct Color : IEquatable<Color>, ISpanFormattable
{
    /// <summary> 红色分量�?Red component. </summary>
    [FieldOffset(0)] public float R;
    /// <summary> 绿色分量�?Green component. </summary>
    [FieldOffset(4)] public float G;
    /// <summary> 蓝色分量�?Blue component. </summary>
    [FieldOffset(8)] public float B;
    /// <summary> 透明度分量�?Alpha component. </summary>
    [FieldOffset(12)] public float A;

    /// <summary> 构造一个新的颜色�?Constructs a new color. </summary>
    public Color(float r, float g, float b, float a = 1f)
    { R = r; G = g; B = b; A = a; }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static ref readonly NumericsVector4 AsNumerics(in Color c) => ref Unsafe.As<Color, NumericsVector4>(ref Unsafe.AsRef(in c));

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static ref Color FromNumerics(ref NumericsVector4 v) => ref Unsafe.As<NumericsVector4, Color>(ref v);

    /// <summary> 白色 (1, 1, 1, 1)�?White. </summary>
    public static readonly Color White = new(1, 1, 1);
    /// <summary> 黑色 (0, 0, 0, 1)�?Black. </summary>
    public static readonly Color Black = new(0, 0, 0);
    /// <summary> 红色 (1, 0, 0, 1)�?Red. </summary>
    public static readonly Color Red   = new(1, 0, 0);
    /// <summary> 绿色 (0, 1, 0, 1)�?Green. </summary>
    public static readonly Color Green = new(0, 1, 0);
    /// <summary> 蓝色 (0, 0, 1, 1)�?Blue. </summary>
    public static readonly Color Blue  = new(0, 0, 1);

    /// <summary> 检查相等性�?Checks for equality. </summary>
    public bool Equals(Color other) => this == other;
    /// <summary> 检查相等性�?Checks for equality. </summary>
    public override bool Equals(object? obj) => obj is Color other && Equals(other);
    /// <summary> 获取哈希码�?Gets the hash code. </summary>
    public override int GetHashCode() => HashCode.Combine(R, G, B, A);

    /// <summary> 相等�?Equality. </summary>
    public static bool operator ==(Color a, Color b) => a.R == b.R && a.G == b.G && a.B == b.B && a.A == b.A;
    /// <summary> 不相等�?Inequality. </summary>
    public static bool operator !=(Color a, Color b) => !(a == b);

    /// <summary> 加法�?Addition. </summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Color operator +(Color a, Color b) { var v = AsNumerics(a) + AsNumerics(b); return FromNumerics(ref v); }
    /// <summary> 减法�?Subtraction. </summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Color operator -(Color a, Color b) { var v = AsNumerics(a) - AsNumerics(b); return FromNumerics(ref v); }
    /// <summary> 乘法�?Multiplication. </summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Color operator *(Color a, float s) { var v = AsNumerics(a) * s; return FromNumerics(ref v); }
    /// <summary> 乘法�?Multiplication. </summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Color operator *(Color a, Color b) { var v = AsNumerics(a) * AsNumerics(b); return FromNumerics(ref v); }

    /// <summary> 线性插值�?Linear interpolation. </summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Color Lerp(Color a, Color b, float t) { var v = NumericsVector4.Lerp(AsNumerics(a), AsNumerics(b), t); return FromNumerics(ref v); }

    /// <summary> 转换为字符串�?Converts to string. </summary>
    public override string ToString() => $"RGBA({R:F2}, {G:F2}, {B:F2}, {A:F2})";

    /// <summary> 零分配格式化支持�?Zero-allocation formatting support. </summary>
    public bool TryFormat(Span<char> destination, out int charsWritten, ReadOnlySpan<char> format = default, IFormatProvider? provider = null)
    {
        return destination.TryWrite(provider, $"RGBA({R:F2}, {G:F2}, {B:F2}, {A:F2})", out charsWritten);
    }

    public string ToString(string? format, IFormatProvider? formatProvider) => ToString();
}
