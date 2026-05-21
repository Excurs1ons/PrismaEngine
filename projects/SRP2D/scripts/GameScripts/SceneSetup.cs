using Prisma;
using Random = Prisma.Random;

namespace SRP2D;

[Serializable]
public partial class SceneSetup : Script
{
    public override void OnCreate()
    {
        for (int i = 0; i < 30; i++)
        {
            var n = Node.Create($"Sprite_{i}");
            n.Position = new Vector2(Random.Range(0, 1920), Random.Range(0, 1080));
            n.Rotation = Random.Range(0, Mathf.PI * 2);

            unsafe
            {
                uint idx = n.Handle & 0xFFFF;
                Interop.RenderData->SizeW[idx] = Random.Range(30, 120);
                Interop.RenderData->SizeH[idx] = Random.Range(30, 120);
                Interop.RenderData->ColorR[idx] = Random.value;
                Interop.RenderData->ColorG[idx] = Random.value;
                Interop.RenderData->ColorB[idx] = Random.value;
                Interop.RenderData->ColorA[idx] = 1.0f;
            }

            n.AddScript<RotatingSprite>();
        }

        CreateLight(960, 540, new Vector3(1.0f, 0.15f, 0.15f), 1.8f, 650f, 1.5f);
        CreateLight(300, 250, new Vector3(0.15f, 0.4f, 1.0f), 1.3f, 450f, 1.0f);
        CreateLight(1600, 750, new Vector3(0.15f, 1.0f, 0.25f), 1.1f, 550f, 2.0f);
        CreateLight(960, 200, new Vector3(0.9f, 0.8f, 0.2f), 0.9f, 350f, 0.8f);

        Debug.Log("SRP2D scene: 30 sprites + 4 point lights.");
    }

    private static void CreateLight(float x, float y, Vector3 color, float intensity, float radius, float falloff)
    {
        var light = new Light2D(LightType.Point);
        light.Position = new Vector2(x, y);
        light.Color = color;
        light.Intensity = intensity;
        light.Radius = radius;
        light.Falloff = falloff;
    }
}

[Serializable]
public partial class RotatingSprite : Script
{
    private float _speed;
    private float _direction;

    public override void OnCreate()
    {
        _speed = Random.Range(0.3f, 2.0f);
        _direction = Random.value > 0.5f ? 1f : -1f;
    }

    public override void OnUpdate(TimeContext time, InputContext input)
    {
        Rotation += _speed * _direction * time.DeltaTime;
    }
}
