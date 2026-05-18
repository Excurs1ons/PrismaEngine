using System;
using Prisma;

namespace GameScripts;

[Serializable]
public partial class CameraController : Script
{
    public float MoveSpeed = 5.0f;
    public float LookSpeed = 0.005f;

    public override void OnUpdate(TimeContext time, InputContext input)
    {
        float horizontal = input.GetAxis(InputContext.HorizontalHash);
        float vertical = input.GetAxis(InputContext.VerticalHash);
    }
}
