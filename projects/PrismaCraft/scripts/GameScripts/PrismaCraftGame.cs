using System;
using Prisma;
using Prisma.SRP;
using GameScripts.Minecraft;

namespace GameScripts;

/// <summary>
/// PrismaCraft 游戏初始化脚本。在 Bootstrap 时运行一次。
/// 创建初始世界状态。
/// </summary>
[Serializable]
public partial class PrismaCraftGame : Script
{
    public override void OnCreate()
    {
        Debug.Log("[PrismaCraftGame] World initializing...");

        // 创建一个测试实体
        var testEntity = Node.Create("test_block");
        testEntity.Position = new Vector2(100, 100);
        testEntity.Scale = new Vector2(50, 50);

        var worldMgr = Node.Create("__WorldManager__");
        worldMgr.AddScript<WorldManager>();

        Debug.Log("[PrismaCraftGame] Ready. SRP pipeline active.");
    }

    public override void OnUpdate(TimeContext time, InputContext input)
    {
        // 游戏逻辑更新
    }
}

/// <summary>
/// 世界管理器——管理 Minecraft World + 区块加载 + 玩家。
/// </summary>
[Serializable]
public partial class WorldManager : Script
{
    public MinecraftWorld MinecraftWorld { get; private set; } = null!;
    public WorldGenerator Generator { get; private set; } = null!;

    // 加载半径（区块）
    public int ViewRadius { get; set; } = 8;
    private ChunkPos _lastCenter = new(int.MaxValue, int.MaxValue);

    public override void OnCreate()
    {
        // 初始化方块注册表
        Blocks.Initialize();

        // 创建世界和生成器
        MinecraftWorld = new MinecraftWorld();
        Generator = new WorldGenerator(Environment.TickCount);

        Debug.Log("[WorldManager] World created with seed: " + Generator.Seed);

        // 在玩家位置周围生成区块
        var spawnChunk = new ChunkPos(0, 0);
        EnsureChunksAround(spawnChunk);
    }

    public override void OnUpdate(TimeContext time, InputContext input)
    {
        MinecraftWorld.Tick();

        // 根据玩家位置加载周围区块
        var playerChunk = new ChunkPos(
            (int)node.X >> 4,
            (int)node.Y >> 4
        );
        if (!playerChunk.Equals(_lastCenter))
        {
            _lastCenter = playerChunk;
            EnsureChunksAround(playerChunk);
        }
    }

    private void EnsureChunksAround(ChunkPos center)
    {
        int loaded = 0;
        for (int dx = -ViewRadius; dx <= ViewRadius; dx++)
        {
            for (int dz = -ViewRadius; dz <= ViewRadius; dz++)
            {
                var pos = new ChunkPos(center.X + dx, center.Z + dz);
                var chunk = MinecraftWorld.GetChunk(pos);
                if (chunk == null || !chunk.IsLoaded)
                {
                    if (chunk == null)
                        chunk = MinecraftWorld.GetOrCreateChunk(pos);
                    Generator.GenerateChunk(chunk);
                    loaded++;
                }
            }
        }
        if (loaded > 0)
            Debug.Log($"[WorldManager] Generated {loaded} new chunks");
    }
}
