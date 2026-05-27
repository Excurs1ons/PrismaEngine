using System;
using Prisma;

namespace GameScripts;

/// 相机控制脚本
/// Camera controller script.
[Serializable]
public partial class CameraController : Script 
{
    public float MoveSpeed = 600f;

    public override void OnUpdate(TimeContext time, InputContext input) 
    {
        // 1. 获取输入（使用预计算哈希，消灭字符串比较开销）
        float horizontal = input.GetAxis(InputContext.HorizontalHash);
        float vertical = input.GetAxis(InputContext.VerticalHash);

        // 2. 获取当前相机坐标
        float x = 0, y = 0;
        unsafe { Interop.API.GetCameraPos(&x, &y); }

        x += horizontal * MoveSpeed * time.DeltaTime;
        y += vertical * MoveSpeed * time.DeltaTime;

        // 3. 应用新的相机坐标
        unsafe { Interop.API.SetCameraPos(x, y); }
        
        // 4. 演示零分配日志 (修正 .NET 10 下的格式化报错)
        if (input.GetKey(KeyCode.Space))
        {
            Debug.Log($"Camera Position: {x}, {y}, DT: {time.DeltaTime}");
        }
    }
}
