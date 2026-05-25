using System;

namespace GameScripts.Minecraft;

不可变方块坐标。对应 net.minecraft.core.BlockPos</summary>
public readonly struct BlockPos : IEquatable<BlockPos>
{
    public readonly int X, Y, Z;

    public BlockPos(int x, int y, int z) { X = x; Y = y; Z = z; }

    // 方向偏移
    public BlockPos Above(int n = 1) => new(X, Y + n, Z);
    public BlockPos Below(int n = 1) => new(X, Y - n, Z);
    public BlockPos North(int n = 1) => new(X, Y, Z - n);
    public BlockPos South(int n = 1) => new(X, Y, Z + n);
    public BlockPos West(int n = 1)  => new(X - n, Y, Z);
    public BlockPos East(int n = 1)  => new(X + n, Y, Z);

    public ChunkPos ToChunkPos() => new(X >> 4, Z >> 4);
    public SectionPos ToSectionPos() => new(X >> 4, Y >> 4, Z >> 4);

    // 区块内局部坐标 (0-15)
    public int LocalX => X & 15;
    public int LocalY => Y & 15;
    public int LocalZ => Z & 15;

    // 64-bit 打包 (26+12+26 bits)
    public long AsLong() => ((long)X & 0x3FFFFFF) << 38 | ((long)Y & 0xFFF) << 26 | (long)Z & 0x3FFFFFF;
    public static BlockPos FromLong(long l) => new((int)(l >> 38), (int)(l << 26 >> 52), (int)(l << 38 >> 38));

    // 区块内线性索引 (y << 8 | z << 4 | x)
    public int ToChunkIndex() => Y << 8 | Z << 4 | X;

    public bool Equals(BlockPos other) => X == other.X && Y == other.Y && Z == other.Z;
    public override bool Equals(object? obj) => obj is BlockPos p && Equals(p);
    public override int GetHashCode() => HashCode.Combine(X, Y, Z);
    public override string ToString() => $"({X},{Y},{Z})";
}

可变 BlockPos，用于迭代。对应 BlockPos.MutableBlockPos</summary>
public struct MutableBlockPos
{
    public int X, Y, Z;
    public MutableBlockPos(int x, int y, int z) { X = x; Y = y; Z = z; }
    public void Set(int x, int y, int z) { X = x; Y = y; Z = z; }
    public void Move(int dx, int dy, int dz) { X += dx; Y += dy; Z += dz; }
    public BlockPos Immutable() => new(X, Y, Z);
}
