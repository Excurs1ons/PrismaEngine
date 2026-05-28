using System;
using System.Runtime.InteropServices;
using Prisma;

namespace GameScripts;

internal static class ScriptEntry
{
    // 给 node 设置渲染数据（彩色矩形，用于 demo）
    static void SetupSprite(Node n, float r, float g, float b, float w = 16, float h = 16)
    {
        unsafe
        {
            uint idx = n.Handle & 0xFFFF;
            Interop.RenderData->Active[idx] = 1;
            Interop.RenderData->SizeW[idx] = w;
            Interop.RenderData->SizeH[idx] = h;
            Interop.RenderData->ColorR[idx] = r;
            Interop.RenderData->ColorG[idx] = g;
            Interop.RenderData->ColorB[idx] = b;
            Interop.RenderData->ColorA[idx] = 1.0f;
        }
    }

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

            // Player (blue rectangle, 12x16)
            var player = Node.Create("Player");
            player.AddScript<PlayerController>();
            SetupSprite(player, 0.2f, 0.4f, 1.0f, 12, 16);

            // Enemy (red rectangle, 14x14)
            var enemy1 = Node.Create("Enemy1");
            enemy1.Position = new Vector2(500, 400);
            var e1 = enemy1.AddScript<Enemy>();
            e1.PatrolLeft = 420;
            e1.PatrolRight = 620;
            SetupSprite(enemy1, 1.0f, 0.2f, 0.2f, 14, 14);

            // Dash pickup (yellow-green diamond)
            var dashPickup = Node.Create("DashPickup");
            dashPickup.Position = new Vector2(960, 380);
            dashPickup.AddScript<DashPickup>();
            SetupSprite(dashPickup, 0.2f, 1.0f, 0.3f, 12, 12);

            // Ability gate (grey wall)
            var gate = Node.Create("AbilityGate");
            gate.Position = new Vector2(640, 432);
            gate.AddScript<AbilityGate>();
            SetupSprite(gate, 0.5f, 0.5f, 0.5f, 32, 32);

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
