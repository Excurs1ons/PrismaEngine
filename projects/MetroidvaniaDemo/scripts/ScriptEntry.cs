using System;
using System.Runtime.InteropServices;
using Prisma;

namespace GameScripts;

internal static class ScriptEntry
{
    [UnmanagedCallersOnly]
    public static void Bootstrap(IntPtr apiPtr)
    {
        try
        {
            ScriptEngine.Bootstrap(apiPtr);
            GameScripts.Generated.ScriptRegistry.RegisterAll();

            // === Scene setup ===

            // 1. Camera follow (anchor node, follows player)
            var camAnchor = Node.Create("__CameraAnchor__");
            camAnchor.AddScript<CameraFollow>();

            // 2. Tilemap collider (loads tilemap, extracts solid AABBs)
            var tilemapNode = Node.Create("__Tilemap__");
            tilemapNode.AddScript<TilemapCollider>();

            // 3. Player
            var player = Node.Create("Player");
            player.AddScript<PlayerController>();

            // 4. Enemy patrols in Room 1
            var enemy1 = Node.Create("Enemy1");
            enemy1.Position = new Vector2(500, 400);
            var e1 = enemy1.AddScript<Enemy>();
            e1.PatrolLeft = 420;
            e1.PatrolRight = 620;

            // 5. Dash pickup in Room 2 (640–1280 x range)
            var dashPickup = Node.Create("DashPickup");
            dashPickup.Position = new Vector2(960, 380);
            dashPickup.AddScript<DashPickup>();

            // 6. Ability gate between Room 1 and Room 2
            var gate = Node.Create("AbilityGate");
            gate.Position = new Vector2(640, 432);
            gate.AddScript<AbilityGate>();

            Console.WriteLine("[MetroidvaniaDemo] Bootstrap complete");
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine("[FATAL] ScriptEntry.Bootstrap: " + ex.GetType().Name + ": " + ex.Message);
            Console.Error.WriteLine(ex.StackTrace);
            throw;
        }
    }

    [UnmanagedCallersOnly]
    public static void OnFrame(float dt)
    {
        ScriptEngine.OnFrame(dt);
    }
}
