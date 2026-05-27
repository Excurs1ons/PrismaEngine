using System;
using Prisma;

namespace GameScripts;

[Serializable]
public partial class CameraFollow : Script
{
    public float LerpSpeed = 6f;

    public override void OnUpdate(TimeContext time, InputContext input)
    {
        Node playerNode = GameState.PlayerNode;
        if (playerNode.Handle == 0) return;

        float targetX = playerNode.X;
        float targetY = playerNode.Y - 8f; // Look slightly up

        float currentX, currentY;
        unsafe { Interop.API.GetCameraPos(&currentX, &currentY); }

        float t = LerpSpeed * time.DeltaTime;
        if (t > 1f) t = 1f;

        float newX = currentX + (targetX - currentX) * t;
        float newY = currentY + (targetY - currentY) * t;

        // Clamp to camera bounds
        newX = Mathf.Clamp(newX, GameState.CameraMinX, GameState.CameraMaxX);
        newY = Mathf.Clamp(newY, GameState.CameraMinY, GameState.CameraMaxY);

        // Pixel snap
        newX = Mathf.Round(newX);
        newY = Mathf.Round(newY);

        unsafe { Interop.API.SetCameraPos(newX, newY); }
    }
}
