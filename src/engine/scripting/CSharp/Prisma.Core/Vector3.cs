using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using NumericsVector3 = System.Numerics.Vector3;

namespace Prisma;

/// <summary>
/// 3D 向量�?
/// 3D Vector.
/// </summary>
[StructLayout(LayoutKind.Explicit)]
public struct Vector3 : IEquatable<Vector3>, ISpanFormattable
{
    /// <summary> X 分量�?X component. </summary>
    [FieldOffset(0)] public float X;
    /// <summary> Y 分量�?Y component. </summary>
    [FieldOffset(4)] public float Y;
    /// <summary> Z 分量�?Z component. </summary>
    [FieldOffset(8)] public float Z;

    /// <summary> 构造一个新�?3D 向量�?Constructs a new 3D vector. </summary>
    public Vector3(float x, float y, float z) { X = x; Y = y; Z = z; }
    /// <summary> 构造一个新�?3D 向量 (Z �?0)�?Constructs a new 3D vector (Z is 0). </summary>
    public Vector3(float x, float y) { X = x; Y = y; Z = 0; }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static ref readonly NumericsVector3 AsNumerics(in Vector3 v) => ref Unsafe.As<Vector3, NumericsVector3>(ref Unsafe.AsRef(in v));

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static ref Vector3 FromNumerics(ref NumericsVector3 v) => ref Unsafe.As<NumericsVector3, Vector3>(ref v);

    /// <summary> 零向�?(0, 0, 0)�?Zero vector (0, 0, 0). </summary>
    public static readonly Vector3 Zero = new(0, 0, 0);
    /// <summary> 单位向量 (1, 1, 1)�?One vector (1, 1, 1). </summary>
    public static readonly Vector3 One = new(1, 1, 1);
    /// <summary> 上向�?(0, 1, 0)�?Up vector (0, 1, 0). </summary>
    public static readonly Vector3 Up = new(0, 1, 0);
    /// <summary> 下向�?(0, -1, 0)�?Down vector (0, -1, 0). </summary>
    public static readonly Vector3 Down = new(0, -1, 0);
    /// <summary> 左向�?(-1, 0, 0)�?Left vector (-1, 0, 0). </summary>
    public static readonly Vector3 Left = new(-1, 0, 0);
    /// <summary> 右向�?(1, 0, 0)�?Right vector (1, 0, 0). </summary>
    public static readonly Vector3 Right = new(1, 0, 0);
    /// <summary> 前向�?(0, 0, 1)�?Forward vector (0, 0, 1). </summary>
    public static readonly Vector3 Forward = new(0, 0, 1);
    /// <summary> 后向�?(0, 0, -1)�?Back vector (0, 0, -1). </summary>
    public static readonly Vector3 Back = new(0, 0, -1);

    /// <summary> 长度的平方�?Squared magnitude. </summary>
    public float SqrMagnitude => AsNumerics(this).LengthSquared();
    /// <summary> 长度�?Magnitude. </summary>
    public float Magnitude => AsNumerics(this).Length();
    /// <summary> 归一化后的向量�?Normalized vector. </summary>
    public Vector3 Normalized { get { var v = NumericsVector3.Normalize(AsNumerics(this)); return FromNumerics(ref v); } }

    /// <summary> 检查相等性�?Checks for equality. </summary>
    public bool Equals(Vector3 other) => this == other;
    /// <summary> 检查相等性�?Checks for equality. </summary>
    public override bool Equals(object? obj) => obj is Vector3 other && Equals(other);
    /// <summary> 获取哈希码�?Gets the hash code. </summary>
    public override int GetHashCode() => HashCode.Combine(X, Y, Z);

    /// <summary> 相等�?Equality. </summary>
    public static bool operator ==(Vector3 a, Vector3 b) => a.X == b.X && a.Y == b.Y && a.Z == b.Z;
    /// <summary> 不相等�?Inequality. </summary>
    public static bool operator !=(Vector3 a, Vector3 b) => !(a == b);

    /// <summary> 加法�?Addition. </summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Vector3 operator +(Vector3 a, Vector3 b) { var v = AsNumerics(a) + AsNumerics(b); return FromNumerics(ref v); }
    /// <summary> 减法�?Subtraction. </summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Vector3 operator -(Vector3 a, Vector3 b) { var v = AsNumerics(a) - AsNumerics(b); return FromNumerics(ref v); }
    /// <summary> 取反�?Negation. </summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Vector3 operator -(Vector3 a) { var v = -AsNumerics(a); return FromNumerics(ref v); }
    /// <summary> 乘法�?Multiplication. </summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Vector3 operator *(Vector3 a, float s) { var v = AsNumerics(a) * s; return FromNumerics(ref v); }
    /// <summary> 乘法�?Multiplication. </summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Vector3 operator *(float s, Vector3 a) { var v = AsNumerics(a) * s; return FromNumerics(ref v); }
    /// <summary> 除法�?Division. </summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Vector3 operator /(Vector3 a, float d) { var v = AsNumerics(a) / d; return FromNumerics(ref v); }

    /// <summary> 点积�?Dot product. </summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static float Dot(Vector3 a, Vector3 b) => NumericsVector3.Dot(AsNumerics(a), AsNumerics(b));
    /// <summary> 叉积�?Cross product. </summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Vector3 Cross(Vector3 a, Vector3 b) { var v = NumericsVector3.Cross(AsNumerics(a), AsNumerics(b)); return FromNumerics(ref v); }

    /// <summary> 距离�?Distance. </summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static float Distance(Vector3 a, Vector3 b) => NumericsVector3.Distance(AsNumerics(a), AsNumerics(b));

    /// <summary> 线性插值�?Linear interpolation. </summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Vector3 Lerp(Vector3 a, Vector3 b, float t) { var v = NumericsVector3.Lerp(AsNumerics(a), AsNumerics(b), t); return FromNumerics(ref v); }

    /// <summary> 转换为字符串�?Converts to string. </summary>
    public override string ToString() => $"({X:F2}, {Y:F2}, {Z:F2})";

    /// <summary> 零分配格式化支持�?Zero-allocation formatting support. </summary>
    public bool TryFormat(Span<char> destination, out int charsWritten, ReadOnlySpan<char> format = default, IFormatProvider? provider = null)
    {
        return destination.TryWrite(provider, $"({X:F2}, {Y:F2}, {Z:F2})", out charsWritten);
    }

    public string ToString(string? format, IFormatProvider? formatProvider) => ToString();
}
