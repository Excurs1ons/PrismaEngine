using System;
using Prisma;

namespace GameScripts;

[Serializable]
public partial class SavePoint : Script
{
    const float ActivationRadius = 30f;
    const float SaveInterval = 2.0f;

    public string SavePointID = "checkpoint";
    bool activated;
    float saveCooldown;

    public override void OnCreate()
    {
        unsafe
        {
            uint idx = node.Handle & 0xFFFF;
            Interop.RenderData->ColorR[idx] = 0.4f;
            Interop.RenderData->ColorG[idx] = 0.4f;
            Interop.RenderData->ColorB[idx] = 0.4f;
        }
    }

    public override void OnUpdate(TimeContext time, InputContext input)
    {
        saveCooldown -= time.DeltaTime;

        Node player = GameState.PlayerNode;
        if (player.Handle == 0) return;

        float dx = node.X - player.X;
        float dy = node.Y - player.Y;
        float distSq = dx * dx + dy * dy;

        if (distSq < ActivationRadius * ActivationRadius)
        {
            if (!activated)
            {
                activated = true;
                unsafe
                {
                    uint idx = node.Handle & 0xFFFF;
                    Interop.RenderData->ColorR[idx] = 0.2f;
                    Interop.RenderData->ColorG[idx] = 0.8f;
                    Interop.RenderData->ColorB[idx] = 0.2f;
                }
                Debug.Log("[SavePoint] Activated: " + SavePointID);
            }

            if (saveCooldown <= 0f)
            {
                GameState.CheckpointPos = new Vector2(node.X, node.Y);
                saveCooldown = SaveInterval;
            }
        }
    }

    public static void LoadCheckpoint()
    {
        Node player = GameState.PlayerNode;
        if (player.Handle == 0) return;

        player.X = GameState.CheckpointPos.X;
        player.Y = GameState.CheckpointPos.Y;

        var pc = player.GetScript<PlayerController>();
        pc?.Respawn();

        Debug.Log("[SavePoint] Loaded checkpoint at (" + GameState.CheckpointPos.X + ", " + GameState.CheckpointPos.Y + ")");
    }
}
