using System;

namespace GameScripts.Minecraft;

/// <summary>简易 Perlin 噪声（用于地形生成）</summary>
public class PerlinNoise
{
    private readonly int[] _perm = new int[512];

    public PerlinNoise(int seed)
    {
        var rng = new Random(seed);
        int[] p = new int[256];
        for (int i = 0; i < 256; i++) p[i] = i;
        // Fisher-Yates shuffle
        for (int i = 255; i > 0; i--)
        {
            int j = rng.Next(i + 1);
            (p[i], p[j]) = (p[j], p[i]);
        }
        for (int i = 0; i < 512; i++) _perm[i] = p[i & 255];
    }

    public double Noise(double x, double y, double z)
    {
        int xi = (int)Math.Floor(x) & 255;
        int yi = (int)Math.Floor(y) & 255;
        int zi = (int)Math.Floor(z) & 255;

        double xf = x - Math.Floor(x);
        double yf = y - Math.Floor(y);
        double zf = z - Math.Floor(z);

        double u = Fade(xf), v = Fade(yf), w = Fade(zf);

        int aaa = _perm[_perm[_perm[xi] + yi] + zi];
        int aba = _perm[_perm[_perm[xi] + yi + 1] + zi];
        int aab = _perm[_perm[_perm[xi] + yi] + zi + 1];
        int abb = _perm[_perm[_perm[xi] + yi + 1] + zi + 1];
        int baa = _perm[_perm[_perm[xi + 1] + yi] + zi];
        int bba = _perm[_perm[_perm[xi + 1] + yi + 1] + zi];
        int bab = _perm[_perm[_perm[xi + 1] + yi] + zi + 1];
        int bbb = _perm[_perm[_perm[xi + 1] + yi + 1] + zi + 1];

        double x1 = Lerp(Grad(aaa, xf, yf, zf), Grad(baa, xf - 1, yf, zf), u);
        double x2 = Lerp(Grad(aba, xf, yf - 1, zf), Grad(bba, xf - 1, yf - 1, zf), u);
        double y1 = Lerp(x1, x2, v);
        x1 = Lerp(Grad(aab, xf, yf, zf - 1), Grad(bab, xf - 1, yf, zf - 1), u);
        x2 = Lerp(Grad(abb, xf, yf - 1, zf - 1), Grad(bbb, xf - 1, yf - 1, zf - 1), u);
        double y2 = Lerp(x1, x2, v);

        return (Lerp(y1, y2, w) + 1.0) / 2.0; // 归一化到 [0,1]
    }

    private static double Fade(double t) => t * t * t * (t * (t * 6 - 15) + 10);
    private static double Lerp(double a, double b, double t) => a + t * (b - a);

    private static double Grad(int hash, double x, double y, double z)
    {
        int h = hash & 15;
        double u = h < 8 ? x : y;
        double v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }
}

/// <summary>多八度噪声组合</summary>
public class OctaveNoise
{
    private readonly PerlinNoise _noise;
    private readonly int _octaves;
    private readonly double _amplitude;
    private readonly double _frequency;
    private readonly double _lacunarity = 2.0;
    private readonly double _persistence = 0.5;

    public OctaveNoise(int seed, int octaves = 4, double amplitude = 1.0, double frequency = 0.01)
    {
        _noise = new PerlinNoise(seed);
        _octaves = octaves;
        _amplitude = amplitude;
        _frequency = frequency;
    }

    public double GetValue(double x, double y, double z)
    {
        double value = 0;
        double amp = _amplitude;
        double freq = _frequency;
        double max = 0;

        for (int i = 0; i < _octaves; i++)
        {
            value += _noise.Noise(x * freq, y * freq, z * freq) * amp;
            max += amp;
            amp *= _persistence;
            freq *= _lacunarity;
        }
        return value / max;
    }
}

/// <summary>地形生成器。对应 Minecraft 的 ChunkGenerator</summary>
public class WorldGenerator
{
    private readonly OctaveNoise _terrainNoise;
    private readonly OctaveNoise _caveNoise;
    private readonly Random _rng;

    public long Seed { get; }
    public int SeaLevel { get; set; } = 64;
    public int BaseHeight { get; set; } = 64;
    public int HeightVariation { get; set; } = 20;

    public WorldGenerator(long seed)
    {
        Seed = seed;
        int s = (int)seed;
        _terrainNoise = new OctaveNoise(s, 4, 1.0, 0.01);
        _caveNoise = new OctaveNoise(s + 1, 3, 1.0, 0.02);
        _rng = new Random(s);
    }

    /// <summary>生成单个区块的地形</summary>
    public void GenerateChunk(LevelChunk chunk)
    {
        int cx = chunk.Position.X;
        int cz = chunk.Position.Z;
        int baseX = cx << 4;
        int baseZ = cz << 4;

        for (int x = 0; x < 16; x++)
        {
            for (int z = 0; z < 16; z++)
            {
                int wx = baseX + x;
                int wz = baseZ + z;

                // 地形高度
                double noise = _terrainNoise.GetValue(wx, 0, wz);
                int height = BaseHeight + (int)(noise * HeightVariation);

                FillColumn(chunk, x, z, height, wx, wz);
            }
        }

        chunk.IsLoaded = true;
        chunk.IsDirty = true;
    }

    private void FillColumn(LevelChunk chunk, int lx, int lz, int height, int wx, int wz)
    {
        for (int y = 0; y < height && y < 256; y++)
        {
            ushort block;

            if (y == 0)
                block = (ushort)(int)BlockId.Bedrock;          // 基岩
            else if (y < height - 4)
                block = (ushort)(int)BlockId.Stone;            // 石头
            else if (y < height - 1)
                block = (ushort)(int)BlockId.Dirt;             // 泥土
            else if (y == height - 1)
                block = (ushort)(int)BlockId.GrassBlock;       // 草
            else
                continue;

            // 洞穴
            if (y > 10 && y < height - 2)
            {
                double cave = _caveNoise.GetValue(wx, y, wz);
                if (cave > 0.65) continue; // 挖空
            }

            chunk.SetBlock(lx, y, lz, block);
        }

        // 海平面以下填充水
        if (height < SeaLevel)
        {
            for (int y = height; y < SeaLevel && y < 256; y++)
            {
                if (chunk.GetBlock(lx, y, lz) == 0)
                    chunk.SetBlock(lx, y, lz, (ushort)(int)BlockId.Water);
            }
        }
    }
}
