using System;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Threading;

namespace PrismaEngine;

/// <summary>
/// 预分配网格空间分割系统（无锁并行安全）。
/// 所有 Cell 的存储空间在构造时一次性分配，更新时仅需 Interlocked.Increment。
/// </summary>
public class SpatialGrid
{
    // 网格大小：32×32 = 1024 个 Cell，覆盖 4096×4096 世界（cellSize=128）
    private const int kGridWidth  = 32;
    private const int kGridHeight = 32;
    private const int kMaxPerCell = 512;   // 每个 Cell 最大容纳实体数

    // 预分配扁平缓冲区：[cellIdx * kMaxPerCell + slot]
    private readonly uint[] _buffer = new uint[kGridWidth * kGridHeight * kMaxPerCell];
    private readonly int[] _counts = new int[kGridWidth * kGridHeight];
    private readonly int _cellSize;

    public SpatialGrid(int cellSize = 128)
    {
        _cellSize = cellSize;
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static int Clamp(int v, int max) => Math.Clamp(v, 0, max - 1);

    /// <summary> 无锁插入：Interlocked.Increment 取槽位，直接写入预分配数组。 </summary>
    public void UpdateEntity(uint handle, Vector2 position)
    {
        int gx = Clamp((int)MathF.Floor(position.X / _cellSize), kGridWidth);
        int gy = Clamp((int)MathF.Floor(position.Y / _cellSize), kGridHeight);
        int cellIdx = gy * kGridWidth + gx;

        int slot = Interlocked.Increment(ref _counts[cellIdx]) - 1;
        if (slot < kMaxPerCell)
            _buffer[cellIdx * kMaxPerCell + slot] = handle;
        // 超出 kMaxPerCell 的实体被静默丢弃（生产中应根据场景调整容量）
    }

    /// <summary> 并行安全的查询（Step 期间网格已重建完毕，不再修改）。 </summary>
    public void Query(Vector2 position, float radius, List<uint> results)
    {
        int xMin = Clamp((int)MathF.Floor((position.X - radius) / _cellSize), kGridWidth);
        int xMax = Clamp((int)MathF.Floor((position.X + radius) / _cellSize), kGridWidth);
        int yMin = Clamp((int)MathF.Floor((position.Y - radius) / _cellSize), kGridHeight);
        int yMax = Clamp((int)MathF.Floor((position.Y + radius) / _cellSize), kGridHeight);

        for (int x = xMin; x <= xMax; x++)
        {
            for (int y = yMin; y <= yMax; y++)
            {
                int cellIdx = y * kGridWidth + x;
                int count = _counts[cellIdx];
                int baseIdx = cellIdx * kMaxPerCell;
                for (int i = 0; i < count; i++)
                    results.Add(_buffer[baseIdx + i]);
            }
        }
    }

    /// <summary> 重置所有 Cell 计数器（不清零预分配数组，仅重置计数）。 </summary>
    public void Clear() => Array.Clear(_counts, 0, _counts.Length);
}
