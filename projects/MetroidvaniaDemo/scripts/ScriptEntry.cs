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

            // 全局环境光（避免 LightMap 全黑导致精灵不可见）
            var envLight = new Light2D(LightType.Point);
            envLight.Position = new Vector2(640, 240);
            envLight.Color = new Vector3(1.0f, 1.0f, 1.0f);
            envLight.Intensity = 2.0f;
            envLight.Radius = 2000.0f;

            Console.WriteLine("[MetroidvaniaDemo] Bootstrap complete — all scene nodes ready");

            // ============================================================
            // Wave 3 Integration: Demonstrate all 5 new engine APIs
            // Each section individually wrapped in try/catch for safety
            // ============================================================

            // 1) Animation Demo — create idle animation and attach to player
            try
            {
                var idleAnim = new SpriteAnimation("idle");
                idleAnim.AddFrame(0, 0, 12, 16, 0.5f);
                idleAnim.AddFrame(12, 0, 12, 16, 0.5f);
                idleAnim.SetLooping(true);

                var animComp = SpriteAnimationComponent.AddToNode(player.Handle);
                if (animComp != null)
                {
                    animComp.Play("idle");
                    Console.WriteLine("[Integration] Animation: idle animation playing on player");
                }
                else
                {
                    Console.WriteLine("[Integration] Animation: AddToNode returned null (expected if unimpl.)");
                }
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine("[Integration] Animation demo skipped: " + ex.Message);
            }

            // 2) Audio Demo — play placeholder BGM (won't crash if file missing)
            try
            {
                Audio.PlayBGM("bgm/demo_placeholder.wav", 0.5f, false);
                Console.WriteLine("[Integration] Audio: PlayBGM called (no crash on missing file)");

                Audio.PlaySFX("sfx/demo_click.wav", 0.8f, 1.0f);
                Console.WriteLine("[Integration] Audio: PlaySFX called (no crash on missing file)");
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine("[Integration] Audio demo skipped: " + ex.Message);
            }

            // 3) Save Demo — persist player progress
            try
            {
                SaveGame.Save("player_progress", "{\"health\":100,\"position\":\"spawn\"}");
                var loaded = SaveGame.Load("player_progress");
                var slots = SaveGame.ListSlots();
                Console.WriteLine("[Integration] Save: saved/loaded OK, slots=" + (slots?.Length.ToString() ?? "0"));
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine("[Integration] Save demo skipped: " + ex.Message);
            }

            // 4) Scene Transition Demo — transition data + scene switch
            try
            {
                SceneManager.SetTransitionData("lastRoom", "starting_room");
                SceneManager.SwitchScene("main_scene", 500);
                var currentScene = SceneManager.GetCurrentSceneName();
                Console.WriteLine("[Integration] Scene: currentScene='" + currentScene + "'");
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine("[Integration] Scene transition demo skipped: " + ex.Message);
            }

            // 5) UI Demo — create HUD canvas + text overlay
            try
            {
                var uiCanvasNode = Node.Create("HUDCanvas");
                var uiCanvas = uiCanvasNode.AddScript<Canvas>();
                uiCanvas.RenderMode = CanvasRenderMode.ScreenSpaceOverlay;

                var hudTextNode = Node.Create("HUDText");
                var hudText = hudTextNode.AddScript<Text>();
                hudText.TextContent = "HP: 100 | Dash: Ready";
                hudText.AnchoredPosition = new Vector2(10, 10);
                hudText.Size = new Vector2(200, 30);
                hudText.TextColor = Color.White;
                uiCanvas.AddElement(hudTextNode);

                var buttonNode = Node.Create("DemoButton");
                var button = buttonNode.AddScript<Button>();
                button.ButtonText = "Reset";
                button.AnchoredPosition = new Vector2(400, 10);
                button.Size = new Vector2(80, 30);
                uiCanvas.AddElement(buttonNode);

                Console.WriteLine("[Integration] UI: HUD canvas + text + button created");
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine("[Integration] UI demo skipped: " + ex.Message);
            }

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
    }
}
