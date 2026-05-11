using System;
using PrismaEngine;

namespace GameScripts;

/// <summary>
/// 相机控制脚本。
/// Camera controller script.
/// </summary>
[Serializable]
public partial class CameraController : Script 
{
    public float MoveSpeed = 600f;

    public override void OnUpdate(TimeContext time, InputContext input) 
    {
        // 1. 获取输入（使用预计算哈希，消灭字符串比较开销）
        float horizontal = input.GetAxis(InputContext.HorizontalHash);
        float vertical = input.GetAxis(InputContext.VerticalHash);

        // 2. 获取旧坐标并计算新坐标 (暂时使用本地变量演示逻辑)
        float x = 960, y = 540;
        // unsafe { NativeAPI.API.GetCameraPos(&x, &y); }

        x += horizontal * MoveSpeed * time.DeltaTime;
        y += vertical * MoveSpeed * time.DeltaTime;

        // 3. 应用
        // unsafe { NativeAPI.API.SetCameraPos(x, y); }
        
        // 4. 演示零分配日志 (修正 .NET 10 下的格式化报错)
        if (input.GetKey(KeyCode.Space))
        {
            Debug.Log($"Camera Position: {x}, {y}, DT: {time.DeltaTime}");
        }
    }
}
