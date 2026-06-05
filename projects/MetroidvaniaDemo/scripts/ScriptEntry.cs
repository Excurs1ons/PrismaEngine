using System;
using System.Runtime.InteropServices;
using Prisma;
using Prisma.Core;
using Prisma.UI;

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
            player.AddScript<PlayerHealth>();
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

            // ── SpriteAnimation Demo (DashPickup pulsing glow) ──
            try
            {
                var dashAnim = new SpriteAnimation("DashPulse");
                dashAnim.AddFrame(0f, 0f, 16f, 16f, 0.15f);
                dashAnim.AddFrame(16f, 0f, 16f, 16f, 0.15f);
                dashAnim.SetLooping(true);
                var animComp = SpriteAnimationComponent.AddToNode(dashPickup.Handle);
                if (animComp != null)
                {
                    animComp.Play("DashPulse", true);
                    animComp.SetSpeed(1.5f);
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine("[SpriteAnimation] Skipped (no spritesheet): " + ex.Message);
            }

            // Ability gate (grey wall)
            var gate = Node.Create("AbilityGate");
            gate.Position = new Vector2(640, 432);
            gate.AddScript<AbilityGate>();
            SetupSprite(gate, 0.5f, 0.5f, 0.5f, 32, 32);

            // 全局环境光（避免 LightMap 全黑导致精灵不可见）
            var envLight = new Light2D(LightType.Point);
            envLight.Position = new Vector2(640, 240);
            envLight.Color = new Vector3(1.0f, 1.0f, 1.0f);
            envLight.Intensity = 2.0f;
            envLight.Radius = 2000.0f;

            // ── 2D Lighting Demo: warm spotlight ──
            var spotLight = new Light2D(LightType.Point);
            spotLight.Position = new Vector2(300, 200);
            spotLight.Color = new Vector3(1.0f, 0.8f, 0.6f);
            spotLight.Intensity = 0.8f;
            spotLight.Radius = 300f;

            // ── 2D Lighting Demo: torch-like orange glow ──
            var torchLight = new Light2D(LightType.Point);
            torchLight.Position = new Vector2(700, 350);
            torchLight.Color = new Vector3(1.0f, 0.5f, 0.2f);
            torchLight.Intensity = 0.6f;
            torchLight.Radius = 200f;

            // Dim ambient so lights are more visible
            Light2D.AmbientColor = new Vector3(0.35f, 0.35f, 0.4f);

            // Register audio zones (rooms in test_dungeon, matching RoomManager.Rooms)
            AudioManager.RegisterZone(0f, 0f, 640f, 480f, "assets/audio/room1_bgm.ogg", 0.7f);
            AudioManager.RegisterZone(640f, 0f, 1280f, 480f, "assets/audio/room2_bgm.ogg", 0.7f);
            AudioManager.RegisterZone(1280f, 0f, 1920f, 480f, "assets/audio/room3_bgm.ogg", 0.7f);

            // ── Integrations: Trigger (DashPickup distance check), Tilemap GPU Instancing (default ON), Audio (above) ──
            // Particle integration: ParticleSystem.Update() is called in OnFrame() below.
            // Particles are triggered on dash via the existing ParticleSystem.Burst/Trail API.

            Console.WriteLine("[MetroidvaniaDemo] Bootstrap complete — all scene nodes ready");

            // HUD (screen-space overlay)
            var hudNode = Node.Create("__HUD__");
            hudNode.AddScript<HUDController>();

            // DoubleJumpPickup in Room 3 high area (x ~80 tiles = 1280 pixels)
            var dJump = Node.Create("DoubleJumpPickup");
            SetupSprite(dJump, 0.8f, 0.2f, 0.8f); // Purple
            dJump.AddScript<DoubleJumpPickup>();
            dJump.X = 1280; // Room 3 high platform
            dJump.Y = 200;  // High up

            // SavePoint in Room 3 safe area
            var savePt = Node.Create("SavePoint");
            SetupSprite(savePt, 0.4f, 0.4f, 0.4f); // Gray (inactive)
            savePt.AddScript<SavePoint>();
            savePt.X = 1120;
            savePt.Y = 432; // Ground level

            // Enemy2 in Room 3
            var enemy2 = Node.Create("Enemy2");
            SetupSprite(enemy2, 1f, 0.3f, 0.3f);
            enemy2.AddScript<Enemy>();
            var e2Ai = enemy2.GetScript<Enemy>();
            // Set patrol range for Room 3 (x ~60-89 tiles = 960-1424 pixels)
            e2Ai.PatrolLeft = 960f;
            e2Ai.PatrolRight = 1424f;
            e2Ai.MoveSpeed = 50f;

            Console.WriteLine("[Bootstrap] Wave 3 integration complete — all 5 APIs demonstrated");
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

        // Advance the local particle system every frame
        ParticleSystem.Update(dt);
    }

    [UnmanagedCallersOnly]
    public static void OnRender(float dt)
    {
        // 2D 渲染管线在 C++ 侧完成，此处可预留 hooks
    }
}
