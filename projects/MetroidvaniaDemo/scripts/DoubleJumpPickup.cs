using System;
using Prisma;

namespace GameScripts;

[Serializable]
public partial class DoubleJumpPickup : Script
{
    const float PickupRadius = 20f;

    bool collected;

    public override void OnUpdate(TimeContext time, InputContext input)
    {
        if (collected || GameState.HasDoubleJump) return;

        Node player = GameState.PlayerNode;
        if (player.Handle == 0) return;

        float dx = node.X - player.X;
        float dy = node.Y - player.Y;
        float distSq = dx * dx + dy * dy;

        if (distSq < PickupRadius * PickupRadius)
        {
            collected = true;
            GameState.HasDoubleJump = true;
            Debug.Log("[MetroidvaniaDemo] Double Jump ability unlocked!");
            node.Destroy();
        }
    }
}
