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
            Console.WriteLine("[Bootstrap] Step 1: ScriptEngine.Bootstrap...");
            ScriptEngine.Bootstrap(apiPtr);
            Console.WriteLine("[Bootstrap] Step 2: ScriptRegistry.RegisterAll...");
            GameScripts.Generated.ScriptRegistry.RegisterAll();
            Console.WriteLine("[Bootstrap] Step 3: creating scene nodes...");

            // === Scene setup ===

            var camAnchor = Node.Create("__CameraAnchor__");
            camAnchor.AddScript<CameraFollow>();

            var tilemapNode = Node.Create("__Tilemap__");
            tilemapNode.AddScript<TilemapCollider>();

            var player = Node.Create("Player");
            player.AddScript<PlayerController>();

            var enemy1 = Node.Create("Enemy1");
            enemy1.Position = new Vector2(500, 400);
            var e1 = enemy1.AddScript<Enemy>();
            e1.PatrolLeft = 420;
            e1.PatrolRight = 620;

            var dashPickup = Node.Create("DashPickup");
            dashPickup.Position = new Vector2(960, 380);
            dashPickup.AddScript<DashPickup>();

            var gate = Node.Create("AbilityGate");
            gate.Position = new Vector2(640, 432);
            gate.AddScript<AbilityGate>();

            Console.WriteLine("[MetroidvaniaDemo] Bootstrap complete — all scene nodes ready");
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
