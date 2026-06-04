using System;

namespace GameScripts.Minecraft;

/// <summary>方块注册表 ID (1:1 对应 BlockState 中的前 12 bits)</summary>
public enum BlockId : ushort
{
    Air = 0,
    Stone = 1,
    GrassBlock = 2,
    Dirt = 3,
    Cobblestone = 4,
    OakPlanks = 5, SprucePlanks, BirchPlanks, JunglePlanks, AcaciaPlanks, DarkOakPlanks,
    Bedrock = 11,
    Water = 12,
    Lava = 13,
    Sand = 14,
    Gravel = 15,
    GoldOre = 16, IronOre, CoalOre, CopperOre,
    OakLog = 20, SpruceLog, BirchLog, JungleLog, AcaciaLog, DarkOakLog,
    OakLeaves = 26, SpruceLeaves, BirchLeaves, JungleLeaves, AcaciaLeaves, DarkOakLeaves,
    Sponge = 32, WetSponge,
    Glass = 34,
    // ... 更多方块
}

方块实例。对应 Minecraft 的 Block 类（每个方块类型只有一个实例）</summary>
public class Block
{
    public BlockId Id { get; }
    public string Name { get; }
    public BlockState DefaultState { get; }
#pragma warning disable CS0649
    internal BlockState[]? StatesByProperty;
#pragma warning restore CS0649

    public float DestroySpeed { get; set; } = 1.0f;
    public float ExplosionResistance { get; set; } = 1.0f;
    public bool IsSolid { get; set; } = true;
    public bool IsAir { get; set; } = false;
    public byte LightEmission { get; set; } = 0;
    public byte LightBlock { get; set; } = 15;

    public Block(BlockId id, string name)
    {
        Id = id;
        Name = name;
        DefaultState = new BlockState(this, (ushort)id);
    }

    public static Block? FromId(ushort id) => Registry.Get((BlockId)id);

方块注册表</summary>
    public static class Registry
    {
        private static readonly Block?[] s_blocks = new Block?[65536];
        private static ushort s_nextId = 1;

        public static Block Register(Block block)
        {
            ushort id = (ushort)(int)block.Id;
            s_blocks[id] = block;
            return block;
        }

        public static Block RegisterNext(BlockId preferred, Block block)
        {
            ushort id = (ushort)(int)preferred;
            if (id >= s_nextId) s_nextId = (ushort)(id + 1);
            s_blocks[id] = block;
            return block;
        }

        public static Block? Get(BlockId id) => s_blocks[(int)id];
        public static Block? Get(ushort id) => s_blocks[id];
    }
}

方块状态（不可变值类型）。对应 Minecraft 的 BlockState</summary>
public readonly struct BlockState : IEquatable<BlockState>
{
压���的 ID：高 4 bits = 属性变体，低 12 bits = BlockId</summary>
    public ushort Id { get; }
    public Block Block { get; }

    public BlockState(Block block, ushort id) { Block = block; Id = id; }
    public BlockState(Block block) { Block = block; Id = (ushort)(int)block.Id; }

    public bool Is(Block block) => Block == block;
    public BlockId BlockId => (BlockId)(Id & 0xFFF);
    public int Variant => Id >> 12;

    public bool Equals(BlockState other) => Id == other.Id && Block == other.Block;
    public override bool Equals(object? obj) => obj is BlockState s && Equals(s);
    public override int GetHashCode() => HashCode.Combine(Id, Block);
    public static bool operator ==(BlockState a, BlockState b) => a.Equals(b);
    public static bool operator !=(BlockState a, BlockState b) => !a.Equals(b);
}

预设方块实例。对应 Minecraft 的 Blocks 类</summary>
public static class Blocks
{
    // 在 World.Initialize() 中填充
    public static Block Air = null!;
    public static Block Stone = null!;
    public static Block GrassBlock = null!;
    public static Block Dirt = null!;
    public static Block Cobblestone = null!;
    public static Block Bedrock = null!;
    public static Block Water = null!;
    public static Block Lava = null!;
    public static Block Sand = null!;
    public static Block Gravel = null!;
    public static Block Glass = null!;

    // 木板
    public static Block OakPlanks = null!, SprucePlanks = null!, BirchPlanks = null!;
    public static Block JunglePlanks = null!, AcaciaPlanks = null!, DarkOakPlanks = null!;

    // 矿石
    public static Block GoldOre = null!, IronOre = null!, CoalOre = null!, CopperOre = null!;

    // 原木
    public static Block OakLog = null!, SpruceLog = null!, BirchLog = null!;
    public static Block JungleLog = null!, AcaciaLog = null!, DarkOakLog = null!;

    // 树叶
    public static Block OakLeaves = null!, SpruceLeaves = null!, BirchLeaves = null!;
    public static Block JungleLeaves = null!, AcaciaLeaves = null!, DarkOakLeaves = null!;

    // 海绵
    public static Block Sponge = null!, WetSponge = null!;

    internal static void Initialize()
    {
        Air = Make(BlockId.Air, "air", b => { b.IsAir = true; b.IsSolid = false; b.LightBlock = 0; });
        Stone = Make(BlockId.Stone, "stone", b => { b.DestroySpeed = 1.5f; b.ExplosionResistance = 6.0f; });
        GrassBlock = Make(BlockId.GrassBlock, "grass_block", b => b.DestroySpeed = 0.6f);
        Dirt = Make(BlockId.Dirt, "dirt", b => b.DestroySpeed = 0.5f);
        Cobblestone = Make(BlockId.Cobblestone, "cobblestone", b => { b.DestroySpeed = 2.0f; b.ExplosionResistance = 6.0f; });
        Bedrock = Make(BlockId.Bedrock, "bedrock", b => b.DestroySpeed = -1f);
        Water = Make(BlockId.Water, "water", b => { b.IsSolid = false; b.LightBlock = 2; });
        Lava = Make(BlockId.Lava, "lava", b => { b.IsSolid = false; b.LightEmission = 15; b.LightBlock = 0; });
        Sand = Make(BlockId.Sand, "sand", b => b.DestroySpeed = 0.5f);
        Gravel = Make(BlockId.Gravel, "gravel", b => b.DestroySpeed = 0.6f);
        Glass = Make(BlockId.Glass, "glass", b => b.LightBlock = 0);

        OakPlanks = Make(BlockId.OakPlanks, "oak_planks", b => b.DestroySpeed = 2.0f);
        SprucePlanks = Make(BlockId.SprucePlanks, "spruce_planks", b => b.DestroySpeed = 2.0f);
        BirchPlanks = Make(BlockId.BirchPlanks, "birch_planks", b => b.DestroySpeed = 2.0f);
        JunglePlanks = Make(BlockId.JunglePlanks, "jungle_planks", b => b.DestroySpeed = 2.0f);
        AcaciaPlanks = Make(BlockId.AcaciaPlanks, "acacia_planks", b => b.DestroySpeed = 2.0f);
        DarkOakPlanks = Make(BlockId.DarkOakPlanks, "dark_oak_planks", b => b.DestroySpeed = 2.0f);

        GoldOre = Make(BlockId.GoldOre, "gold_ore", b => { b.DestroySpeed = 3.0f; b.ExplosionResistance = 3.0f; });
        IronOre = Make(BlockId.IronOre, "iron_ore", b => { b.DestroySpeed = 3.0f; b.ExplosionResistance = 3.0f; });
        CoalOre = Make(BlockId.CoalOre, "coal_ore", b => { b.DestroySpeed = 3.0f; b.ExplosionResistance = 3.0f; });
        CopperOre = Make(BlockId.CopperOre, "copper_ore", b => { b.DestroySpeed = 3.0f; b.ExplosionResistance = 3.0f; });

        OakLog = Make(BlockId.OakLog, "oak_log", b => b.DestroySpeed = 2.0f);
        SpruceLog = Make(BlockId.SpruceLog, "spruce_log", b => b.DestroySpeed = 2.0f);
        BirchLog = Make(BlockId.BirchLog, "birch_log", b => b.DestroySpeed = 2.0f);
        JungleLog = Make(BlockId.JungleLog, "jungle_log", b => b.DestroySpeed = 2.0f);
        AcaciaLog = Make(BlockId.AcaciaLog, "acacia_log", b => b.DestroySpeed = 2.0f);
        DarkOakLog = Make(BlockId.DarkOakLog, "dark_oak_log", b => b.DestroySpeed = 2.0f);

        OakLeaves = Make(BlockId.OakLeaves, "oak_leaves", b => { b.DestroySpeed = 0.2f; b.IsSolid = false; b.LightBlock = 1; });
        SpruceLeaves = Make(BlockId.SpruceLeaves, "spruce_leaves", b => { b.DestroySpeed = 0.2f; b.IsSolid = false; b.LightBlock = 1; });
        BirchLeaves = Make(BlockId.BirchLeaves, "birch_leaves", b => { b.DestroySpeed = 0.2f; b.IsSolid = false; b.LightBlock = 1; });
        JungleLeaves = Make(BlockId.JungleLeaves, "jungle_leaves", b => { b.DestroySpeed = 0.2f; b.IsSolid = false; b.LightBlock = 1; });
        AcaciaLeaves = Make(BlockId.AcaciaLeaves, "acacia_leaves", b => { b.DestroySpeed = 0.2f; b.IsSolid = false; b.LightBlock = 1; });
        DarkOakLeaves = Make(BlockId.DarkOakLeaves, "dark_oak_leaves", b => { b.DestroySpeed = 0.2f; b.IsSolid = false; b.LightBlock = 1; });

        Sponge = Make(BlockId.Sponge, "sponge", b => b.DestroySpeed = 0.6f);
        WetSponge = Make(BlockId.WetSponge, "wet_sponge", b => b.DestroySpeed = 0.6f);
    }

    private static Block Register(BlockId id, string name)
    {
        var block = new Block(id, name);
        Block.Registry.Register(block);
        return block;
    }

    private static Block Make(BlockId id, string name, Action<Block> setup)
    {
        var block = Register(id, name);
        setup(block);
        return block;
    }
}
