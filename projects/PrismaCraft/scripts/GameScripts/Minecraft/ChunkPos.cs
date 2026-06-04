using System;

namespace GameScripts.Minecraft;

/// <summary>区块坐标。对应 net.minecraft.core.ChunkPos</summary>
public readonly struct ChunkPos : IEquatable<ChunkPos>
{
    public readonly int X, Z;
    public ChunkPos(int x, int z) { X = x; Z = z; }

    public BlockPos GetBlockAt(int x, int y, int z) => new(X * 16 + x, y, Z * 16 + z);
    public int MinBlockX => X * 16;
    public int MinBlockZ => Z * 16;
    public int MaxBlockX => X * 16 + 15;
    public int MaxBlockZ => Z * 16 + 15;

    public long AsLong() => (long)X << 32 | (uint)Z;
    public static ChunkPos FromLong(long l) => new((int)(l >> 32), (int)l);

    public bool Equals(ChunkPos other) => X == other.X && Z == other.Z;
    public override bool Equals(object? obj) => obj is ChunkPos p && Equals(p);
    public override int GetHashCode() => HashCode.Combine(X, Z);
    public override string ToString() => $"[{X},{Z}]";
}

Section (16x16x16) 坐标。对应 net.minecraft.core.SectionPos</summary>
public readonly struct SectionPos : IEquatable<SectionPos>
{
    public readonly int X, Y, Z;
    public SectionPos(int x, int y, int z) { X = x; Y = y; Z = z; }

    public ChunkPos ToChunkPos() => new(X, Z);
    public BlockPos Origin() => new(X * 16, Y * 16, Z * 16);
    public BlockPos GetBlockAt(int dx, int dy, int dz) => new(X * 16 + dx, Y * 16 + dy, Z * 16 + dz);

    public long AsLong() => (long)(X & 0x3FFFFF) << 42 | (long)(Y & 0xFFFFF) << 20 | (long)(Z & 0x3FFFFF);
    public static SectionPos FromLong(long l) => new((int)(l >> 42), (int)(l << 44 >> 44), (int)(l << 22 >> 42));

    public bool Equals(SectionPos other) => X == other.X && Y == other.Y && Z == other.Z;
    public override bool Equals(object? obj) => obj is SectionPos p && Equals(p);
    public override int GetHashCode() => HashCode.Combine(X, Y, Z);
}
