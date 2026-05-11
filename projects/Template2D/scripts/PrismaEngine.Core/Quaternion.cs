using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using NumericsQuaternion = System.Numerics.Quaternion;

namespace PrismaEngine;

/// <summary>
/// 四元数，用于表示旋转。
/// Quaternion, used to represent rotations.
/// </summary>
[StructLayout(LayoutKind.Explicit)]
public struct Quaternion : IEquatable<Quaternion>
{
    /// <summary>X 分量。 X component.</summary>
    [FieldOffset(0)] public float X;
    /// <summary>Y 分量。 Y component.</summary>
    [FieldOffset(4)] public float Y;
    /// <summary>Z 分量。 Z component.</summary>
    [FieldOffset(8)] public float Z;
    /// <summary>W 分量。 W component.</summary>
    [FieldOffset(12)] public float W;

    /// <summary> 使用指定的组件创建四元数。 Creates a quaternion with the specified components. </summary>
    public Quaternion(float x, float y, float z, float w) { X = x; Y = y; Z = z; W = w; }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static ref readonly NumericsQuaternion AsNumerics(in Quaternion q) => ref Unsafe.As<Quaternion, NumericsQuaternion>(ref Unsafe.AsRef(in q));

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static ref Quaternion FromNumerics(ref NumericsQuaternion v) => ref Unsafe.As<NumericsQuaternion, Quaternion>(ref v);

    /// <summary> 单位四元数（无旋转）。 The identity quaternion (no rotation). </summary>
    public static readonly Quaternion Identity = new(0, 0, 0, 1);

    /// <summary>判断是否相等。 Checks for equality.</summary>
    public bool Equals(Quaternion other) => this == other;
    /// <summary>判断是否相等。 Checks for equality.</summary>
    public override bool Equals(object? obj) => obj is Quaternion other && Equals(other);
    /// <summary>获取哈希码。 Gets the hash code.</summary>
    public override int GetHashCode() => HashCode.Combine(X, Y, Z, W);

    /// <summary> 相等。 Equality. </summary>
    public static bool operator ==(Quaternion a, Quaternion b) => a.X == b.X && a.Y == b.Y && a.Z == b.Z && a.W == b.W;
    /// <summary> 不相等。 Inequality. </summary>
    public static bool operator !=(Quaternion a, Quaternion b) => !(a == b);

    /// <summary> 乘法运算符。 Multiplication operator. </summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Quaternion operator *(Quaternion lhs, Quaternion rhs) { var v = AsNumerics(lhs) * AsNumerics(rhs); return FromNumerics(ref v); }

    /// <summary> 使用四元数旋转向量。 Rotates a vector by a quaternion. </summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Vector3 operator *(Quaternion rotation, Vector3 point) 
    { 
        // System.Numerics.Vector3 旋转通过 Vector3.Transform 实现
        var v = System.Numerics.Vector3.Transform(Unsafe.As<Vector3, System.Numerics.Vector3>(ref point), AsNumerics(rotation));
        return Unsafe.As<System.Numerics.Vector3, Vector3>(ref v);
    }

    /// <summary> 从欧拉角创建四元数（角度制）。 Creates a quaternion from Euler angles (in degrees). </summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static Quaternion Euler(float x, float y, float z)
    {
        var v = NumericsQuaternion.CreateFromYawPitchRoll(y * Mathf.Deg2Rad, x * Mathf.Deg2Rad, z * Mathf.Deg2Rad);
        return FromNumerics(ref v);
    }

    /// <summary> 从欧拉角向量创建四元数（角度制）。 Creates a quaternion from an Euler angles vector (in degrees). </summary>
    public static Quaternion Euler(Vector3 euler) => Euler(euler.X, euler.Y, euler.Z);

    /// <summary>转换为字符串。 Returns a string representation.</summary>
    public override string ToString() => $"({X:F2}, {Y:F2}, {Z:F2}, {W:F2})";
}
