using PrismaEngine;

namespace GameScripts;

/// <summary>
/// 场景入口脚本：创建摄像机 + 20 个动态旋转精灵。
/// </summary>
public class SceneInit : Script
{
    public override void OnCreate()
    {
        // 摄像机 anchor
        var camAnchor = new Node("__CameraAnchor__");
        camAnchor.AddScript<CameraController>();

        // 20 个动态精灵
        for (int i = 0; i < 20; i++)
        {
            var n = new Node($"Sprite_{i}");
            n.Position = new Vector2(Random.Range(0, 1920), Random.Range(0, 1080));
            n.Rotation = Random.Range(0, 360);

            // Size 和 Color 没有对应的 Node 属性，走 raw API
            unsafe
            {
                NativeAPI.API.SetSize(n._handle, Random.Range(40, 100), Random.Range(40, 100));
                NativeAPI.API.SetColor(n._handle,
                    (float)Random.Range(0, 100) / 100.0f,
                    (float)Random.Range(0, 100) / 100.0f,
                    (float)Random.Range(0, 100) / 100.0f, 1);
            }

            n.AddScript<RotatingSprite>();
        }
    }
}
