using System;
using Prisma;
using Random = Prisma.Random;

namespace GameScripts;

/// <summary>
/// 旋转精灵脚本�?
/// Rotating sprite script.
/// </summary>
[Serializable]
public partial class RotatingSprite : Script
{
    public float Speed = 45f;

    public override void OnCreate()
    {
        // 初始旋转和随机速度
        Rotation = Random.Range(0, Mathf.PI * 2);
        Speed = Random.Range(1f, 3f) * (Random.Range(0, 2) == 0 ? 1 : -1);
    }

    public override void OnUpdate(TimeContext time, InputContext input)
    {
        Rotation += Speed * time.DeltaTime;
        // 自动循环处理由底层或业务按需决定，此处保持简�?
    }
}
