using System;
using Prisma;

namespace GameScripts;

[Serializable]
public partial class CameraFollow : Script
{
    public float LerpSpeed = 6f;

    // Room camera bounds (updated each frame by RoomManager)
    public bool LockToRoom = true;
    public float RoomLeft = 0f, RoomTop = 0f, RoomRight = 640f, RoomBottom = 480f;

    public override void OnCreate()
    {
        // 将相机初始位置设为玩家出生点，避免前几帧玩家在屏幕外
        unsafe { Interop.API.SetCameraPos(64, 400); }
    }

    public override void OnUpdate(TimeContext time, InputContext input)
    {
        Node playerNode = GameState.PlayerNode;
        if (playerNode.Handle == 0) return;

        // 先更新 RoomManager（检测房间切换）
        RoomManager.Update(time, input);

        float targetX = playerNode.X;
        float targetY = playerNode.Y - 8f; // Look slightly up

        float currentX, currentY;
        unsafe { Interop.API.GetCameraPos(&currentX, &currentY); }

        float t = LerpSpeed * time.DeltaTime;
        if (t > 1f) t = 1f;

        float newX = currentX + (targetX - currentX) * t;
        float newY = currentY + (targetY - currentY) * t;

        // Room camera clamping: 限制相机在房间边界内
        if (LockToRoom)
        {
            RoomManager.GetCurrentRoomBounds(out float rX, out float rY, out float rW, out float rH);
            float halfViewW = 128f;  // Half the ortho camera width (256/2)
            float halfViewH = 112f;  // Half the ortho camera height (224/2)
            newX = Mathf.Clamp(newX, rX + halfViewW, rX + rW - halfViewW);
            newY = Mathf.Clamp(newY, rY + halfViewH, rY + rH - halfViewH);
        }

        // Pixel snap
        newX = Mathf.Round(newX);
        newY = Mathf.Round(newY);

        unsafe { Interop.API.SetCameraPos(newX, newY); }
    }
}
