using System;
using PrismaEngine;
using Random = PrismaEngine.Random;
namespace GameScripts;

[Serializable]
public partial class SceneInit : Script
{
    public override void OnCreate()
    {
        // 1. 创建相机 (现代 Handle 模式)
        var camAnchor = Node.Create("__CameraAnchor__");
        camAnchor.AddScript<CameraController>();

        // 2. 创建 20 个动态精灵
        for (int i = 0; i < 20; i++)
        {
            var n = Node.Create($"Sprite_{i}");
            
            n.Position = new Vector2(Random.Range(0, 1920), Random.Range(0, 1080));
            n.Rotation = Random.Range(0, Mathf.PI * 2);

            unsafe
            {
                uint idx = n.Handle & 0xFFFF;
                NativeAPI.RenderBuffer->SizeW[idx] = Random.Range(40, 100);
                NativeAPI.RenderBuffer->SizeH[idx] = Random.Range(40, 100);
                
                NativeAPI.RenderBuffer->ColorR[idx] = Random.value;
                NativeAPI.RenderBuffer->ColorG[idx] = Random.value;
                NativeAPI.RenderBuffer->ColorB[idx] = Random.value;
                NativeAPI.RenderBuffer->ColorA[idx] = 1.0f;
            }

            n.AddScript<RotatingSprite>();
        }
        
        Debug.Log("Scene initialized (Industrial Double-Buffered Architecture).");
    }
}
