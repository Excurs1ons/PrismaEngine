using System;
using Prisma;
using Prisma.SRP;

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
/// 世界管理器——管理区块、玩家、世界状态。
/// 后续将包含 ChunkManager、WorldGenerator 等。
/// </summary>
[Serializable]
public partial class WorldManager : Script
{
    public int WorldTime { get; set; }
    public float RainStrength { get; set; }

    public override void OnCreate()
    {
        Debug.Log("[WorldManager] World created");
    }

    public override void OnUpdate(TimeContext time, InputContext input)
    {
        WorldTime = (WorldTime + 1) % 24000;
    }
}
