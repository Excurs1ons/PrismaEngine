using System;

namespace PrismaEngine;

/// <summary>
/// 2D 向量。
/// </summary>
public struct Vector2 : IEquatable<Vector2>
{
    public float X;
    public float Y;

    public Vector2(float x, float y) { X = x; Y = y; }

    public static Vector2 Zero => new(0, 0);
    public static Vector2 One  => new(1, 1);

    public bool Equals(Vector2 other) => X == other.X && Y == other.Y;

    public override bool Equals(object? obj)
        => obj is Vector2 other && Equals(other);

    public override int GetHashCode() => HashCode.Combine(X, Y);

    public static Vector2 operator +(Vector2 a, Vector2 b) => new(a.X + b.X, a.Y + b.Y);
    public static Vector2 operator -(Vector2 a, Vector2 b) => new(a.X - b.X, a.Y - b.Y);
    public static Vector2 operator *(Vector2 a, float s)   => new(a.X * s, a.Y * s);
    public static Vector2 operator /(Vector2 a, float d)   => new(a.X / d, a.Y / d);

    public override string ToString() => $"({X:F2}, {Y:F2})";
}
