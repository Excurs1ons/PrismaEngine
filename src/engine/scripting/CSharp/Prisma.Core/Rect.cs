using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using NumericsVector4 = System.Numerics.Vector4;

namespace Prisma;

/// <summary>
/// 2D 矩形�?
/// 2D Rectangle.
/// </summary>
[StructLayout(LayoutKind.Explicit)]
public struct Rect : IEquatable<Rect>
{
    /// <summary> X 坐标�?X coordinate. </summary>
    [FieldOffset(0)] public float X;
    /// <summary> Y 坐标�?Y coordinate. </summary>
    [FieldOffset(4)] public float Y;
    /// <summary> 宽度�?Width. </summary>
    [FieldOffset(8)] public float Width;
    /// <summary> 高度�?Height. </summary>
    [FieldOffset(12)] public float Height;

    /// <summary> 构造一个新的矩形�?Constructs a new rectangle. </summary>
    public Rect(float x, float y, float width, float height)
    { X = x; Y = y; Width = width; Height = height; }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static ref readonly NumericsVector4 AsNumerics(in Rect r) => ref Unsafe.As<Rect, NumericsVector4>(ref Unsafe.AsRef(in r));

    /// <summary> 最�?X 坐标�?Minimum X coordinate. </summary>
    public float xMin { [MethodImpl(MethodImplOptions.AggressiveInlining)] get => X; [MethodImpl(MethodImplOptions.AggressiveInlining)] set { float num = xMax; X = value; Width = num - X; } }
    /// <summary> 最�?Y 坐标�?Minimum Y coordinate. </summary>
    public float yMin { [MethodImpl(MethodImplOptions.AggressiveInlining)] get => Y; [MethodImpl(MethodImplOptions.AggressiveInlining)] set { float num = yMax; Y = value; Height = num - Y; } }
    /// <summary> 最�?X 坐标�?Maximum X coordinate. </summary>
    public float xMax { [MethodImpl(MethodImplOptions.AggressiveInlining)] get => X + Width; [MethodImpl(MethodImplOptions.AggressiveInlining)] set => Width = value - X; }
    /// <summary> 最�?Y 坐标�?Maximum Y coordinate. </summary>
    public float yMax { [MethodImpl(MethodImplOptions.AggressiveInlining)] get => Y + Height; [MethodImpl(MethodImplOptions.AggressiveInlining)] set => Height = value - Y; }

    /// <summary> 矩形中心点�?Center point of the rectangle. </summary>
    public Vector2 Center
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)] get => new(X + Width * 0.5f, Y + Height * 0.5f);
        [MethodImpl(MethodImplOptions.AggressiveInlining)] set { X = value.X - Width * 0.5f; Y = value.Y - Height * 0.5f; }
    }

    /// <summary> 矩形左下角位置�?Position of the bottom-left corner. </summary>
    public Vector2 Position
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)] get => new(X, Y);
        [MethodImpl(MethodImplOptions.AggressiveInlining)] set { X = value.X; Y = value.Y; }
    }

    /// <summary> 矩形大小�?Size of the rectangle. </summary>
    public Vector2 Size
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)] get => new(Width, Height);
        [MethodImpl(MethodImplOptions.AggressiveInlining)] set { Width = value.X; Height = value.Y; }
    }

    /// <summary> 检查点是否在矩形内�?Checks if a point is inside the rectangle. </summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public bool Contains(Vector2 point) => point.X >= xMin && point.X < xMax && point.Y >= yMin && point.Y < yMax;

    /// <summary> 检查两个矩形是否重叠�?Checks if two rectangles overlap. </summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public bool Overlaps(Rect other) => other.xMax > xMin && other.xMin < xMax && other.yMax > yMin && other.yMin < yMax;

    /// <summary> 检查相等性�?Checks for equality. </summary>
    public bool Equals(Rect other) => this == other;
    /// <summary> 检查相等性�?Checks for equality. </summary>
    public override bool Equals(object? obj) => obj is Rect other && Equals(other);
    /// <summary> 获取哈希码�?Gets the hash code. </summary>
    public override int GetHashCode() => HashCode.Combine(X, Y, Width, Height);

    /// <summary> 相等�?Equality. </summary>
    public static bool operator ==(Rect a, Rect b) => a.X == b.X && a.Y == b.Y && a.Width == b.Width && a.Height == b.Height;
    /// <summary> 不相等�?Inequality. </summary>
    public static bool operator !=(Rect a, Rect b) => !(a == b);

    /// <summary> 转换为字符串�?Converts to string. </summary>
    public override string ToString() => $"(x:{X:F2}, y:{Y:F2}, width:{Width:F2}, height:{Height:F2})";
}
