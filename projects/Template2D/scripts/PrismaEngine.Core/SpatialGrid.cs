using System;
using System.Collections.Generic;
using System.Runtime.CompilerServices;

namespace PrismaEngine;

/// <summary>
/// 网格空间分割系统。
/// Grid-based Spatial Partitioning System.
/// </summary>
public class SpatialGrid
{
    private readonly int _cellSize;
    private readonly Dictionary<long, List<uint>> _grid = new();

    public SpatialGrid(int cellSize = 128)
    {
        _cellSize = cellSize;
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static long GetKey(int x, int y) => ((long)x << 32) | (uint)y;

    /// <summary> 批量更新接口。 </summary>
    public void UpdateEntity(uint handle, Vector2 position)
    {
        int gx = (int)MathF.Floor(position.X / _cellSize);
        int gy = (int)MathF.Floor(position.Y / _cellSize);
        long key = GetKey(gx, gy);

        if (!_grid.TryGetValue(key, out var list))
        {
            list = new List<uint>();
            _grid[key] = list;
        }
        list.Add(handle);
    }

    /// <summary>
    /// 并行安全的查询。
    /// 因为网格在 Step 开始前已经重建完成且不再修改。
    /// </summary>
    public void Query(Vector2 position, float radius, List<uint> results)
    {
        int xMin = (int)MathF.Floor((position.X - radius) / _cellSize);
        int xMax = (int)MathF.Floor((position.X + radius) / _cellSize);
        int yMin = (int)MathF.Floor((position.Y - radius) / _cellSize);
        int yMax = (int)MathF.Floor((position.Y + radius) / _cellSize);

        for (int x = xMin; x <= xMax; x++)
        {
            for (int y = yMin; y <= yMax; y++)
            {
                if (_grid.TryGetValue(GetKey(x, y), out var list))
                {
                    // 警告：如果结果列表由外部传入且非线程局部，则此处仍有竞态风险。
                    // 建议每个线程拥有自己的结果列表。
                    results.AddRange(list);
                }
            }
        }
    }

    public void Clear() => _grid.Clear();
}
