using System;
using System.Collections.Generic;

namespace GameScripts.Minecraft;

/// <summary>16x16x16 切片。对应 Minecraft 的 ChunkSection</summary>
public class ChunkSection
{
    public const int Size = 16;
    public const int BlockCount = Size * Size * Size; // 4096

    public int Y { get; }
    public bool IsEmpty { get; private set; } = true;

    // TODO: PalettedContainer 优化
    private readonly ushort[] _blocks = new ushort[BlockCount];

    public ChunkSection(int y) { Y = y; }

    public ushort GetBlock(int x, int y, int z)
    {
        return _blocks[Index(x, y, z)];
    }

    public void SetBlock(int x, int y, int z, ushort blockId)
    {
        int idx = Index(x, y, z);
        _blocks[idx] = blockId;
        if (blockId != 0) IsEmpty = false;
    }

    public void Fill(ushort blockId)
    {
        Array.Fill(_blocks, blockId);
        IsEmpty = blockId == 0;
    }

    public void Clear() { Array.Clear(_blocks); IsEmpty = true; }

    public ushort[] GetRawData() => _blocks;

    private static int Index(int x, int y, int z) => y << 8 | z << 4 | x;
}

/// <summary>区块。对应 Minecraft 的 LevelChunk</summary>
public class LevelChunk
{
    public ChunkPos Position { get; }
    public bool IsLoaded { get; set; }
    public bool IsDirty { get; set; } = true;

    private readonly ChunkSection[] _sections = new ChunkSection[16]; // 16x16x256

    // 高度图：每列最高方块 Y
    public ushort[] HeightMap { get; } = new ushort[256];

    public LevelChunk(ChunkPos pos)
    {
        Position = pos;
        for (int i = 0; i < 16; i++)
            _sections[i] = new ChunkSection(i);
    }

    public ChunkSection GetSection(int sectionY)
    {
        if (sectionY < 0 || sectionY >= 16) return null!;
        return _sections[sectionY];
    }

    public ushort GetBlock(int x, int y, int z)
    {
        int sectionY = y >> 4;
        if (sectionY < 0 || sectionY >= 16) return 0;
        return _sections[sectionY].GetBlock(x, y & 15, z);
    }

    public void SetBlock(int x, int y, int z, ushort blockId)
    {
        int sectionY = y >> 4;
        if (sectionY < 0 || sectionY >= 16) return;
        _sections[sectionY].SetBlock(x, y & 15, z, blockId);
        IsDirty = true;

        // 更新高度图
        if (blockId != 0 && y > HeightMap[z << 4 | x])
            HeightMap[z << 4 | x] = (ushort)y;
        else if (blockId == 0 && y == HeightMap[z << 4 | x])
            RecalcHeight(x, z);
    }

    private void RecalcHeight(int x, int z)
    {
        int idx = z << 4 | x;
        HeightMap[idx] = 0;
        for (int y = 255; y >= 0; y--)
        {
            if (GetBlock(x, y, z) != 0) { HeightMap[idx] = (ushort)y; break; }
        }
    }
}

/// <summary>世界。对应 Minecraft 的 ServerLevel / Level</summary>
public class MinecraftWorld
{
    public const int SeaLevel = 64;
    public const int MaxHeight = 256;

    private readonly Dictionary<long, LevelChunk> _chunks = new();

    public int WorldTime { get; set; }
    public float RainStrength { get; set; }
    public float ThunderStrength { get; set; }

    /// <summary>获取或创建区块</summary>
    public LevelChunk GetOrCreateChunk(ChunkPos pos)
    {
        long key = pos.AsLong();
        if (!_chunks.TryGetValue(key, out var chunk))
        {
            chunk = new LevelChunk(pos);
            _chunks[key] = chunk;
        }
        return chunk;
    }

    public LevelChunk? GetChunk(ChunkPos pos)
    {
        _chunks.TryGetValue(pos.AsLong(), out var chunk);
        return chunk;
    }

    public ushort GetBlock(BlockPos pos)
    {
        var chunk = GetChunk(pos.ToChunkPos());
        if (chunk == null) return 0;
        return chunk.GetBlock(pos.LocalX, pos.Y, pos.LocalZ);
    }

    public void SetBlock(BlockPos pos, ushort blockId)
    {
        var chunk = GetOrCreateChunk(pos.ToChunkPos());
        chunk.SetBlock(pos.LocalX, pos.Y, pos.LocalZ, blockId);
    }

    public bool IsAir(BlockPos pos) => GetBlock(pos) == 0;

    /// <summary>获取最高方块 Y（含空气判断）</summary>
    public int GetHeight(int x, int z)
    {
        var chunk = GetChunk(new ChunkPos(x >> 4, z >> 4));
        if (chunk == null) return 0;
        return chunk.HeightMap[(z & 15) << 4 | (x & 15)];
    }

    public IEnumerable<LevelChunk> GetAllChunks() => _chunks.Values;
    public int LoadedChunkCount => _chunks.Count;

    /// <summary>更新世界状态（每帧调用）</summary>
    public void Tick()
    {
        WorldTime = (WorldTime + 1) % 24000;
    }
}
