using PrismaEngine;

namespace GameScripts;

public class RotatingSprite : Script
{
    public float Speed = 45f;

    public override void OnCreate()
    {
        Rotation = Random.Range(0, 360);
        Speed = Random.Range(30, 120) * (Random.Range(0, 2) == 0 ? 1 : -1);
    }

    public override void OnUpdate(float dt)
    {
        Rotation += Speed * dt;
        if (Rotation > 360) Rotation -= 360;
    }
}
