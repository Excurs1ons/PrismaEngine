using System;
using Prisma;
using Random = Prisma.Random;
namespace GameScripts;

[Serializable]
public partial class SceneInit : Script
{
    public override void OnCreate()
    {
        // 1. 创建相机 (现代 Handle 模式)
        var camAnchor = Node.Create("__CameraAnchor__");
        camAnchor.AddScript<CameraController>();

        // 2. 创建 20 个动态精�?
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
        
        // 3. 创建 2D 点光源演示
        var lightRed = new Light2D(LightType.Point);
        lightRed.Position = new Vector2(960, 540);
        lightRed.Color = new Vector3(1.0f, 0.2f, 0.2f);
        lightRed.Intensity = 1.5f;
        lightRed.Radius = 600.0f;
        lightRed.Falloff = 1.5f;

        var lightBlue = new Light2D(LightType.Point);
        lightBlue.Position = new Vector2(300, 300);
        lightBlue.Color = new Vector3(0.2f, 0.4f, 1.0f);
        lightBlue.Intensity = 1.2f;
        lightBlue.Radius = 400.0f;
        lightBlue.Falloff = 1.0f;

        var lightGreen = new Light2D(LightType.Point);
        lightGreen.Position = new Vector2(1600, 700);
        lightGreen.Color = new Vector3(0.2f, 1.0f, 0.3f);
        lightGreen.Intensity = 1.0f;
        lightGreen.Radius = 500.0f;
        lightGreen.Falloff = 2.0f;
        
        // 4. 鼠标跟随光源
        var mouseLightNode = Node.Create("__MouseLight__");
        mouseLightNode.AddScript<MouseLight>();

        Debug.Log("Scene initialized with 3 PointLight2D + mouse tracking light.");
    }
}
