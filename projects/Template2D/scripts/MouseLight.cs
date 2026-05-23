using System;
using Prisma;

namespace GameScripts;

/// <summary>
/// 点光绑定到鼠标位置，实时跟随移动
/// </summary>
[Serializable]
public partial class MouseLight : Script
{
    private Light2D _light;

    public override void OnCreate()
    {
        _light = new Light2D(LightType.Point);
        _light.Color = new Vector3(1.0f, 0.8f, 0.4f); // 暖色灯光
        _light.Intensity = 2.0f;
        _light.Radius = 500.0f;
        _light.Falloff = 1.5f;

        Debug.Log("MouseLight created, move mouse to see light follow.");
    }

    public override void OnUpdate(TimeContext time, InputContext input)
    {
        if (_light == null) return;
        _light.Position = input.MousePosition;
    }

    public override void OnDestroy()
    {
        _light?.Dispose();
        _light = null;
    }
}
