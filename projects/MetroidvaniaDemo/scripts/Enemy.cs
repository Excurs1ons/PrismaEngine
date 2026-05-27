using System;
using Prisma;

namespace GameScripts;

[Serializable]
public partial class Enemy : Script
{
    const float Gravity = 980f;
    const float MaxFallSpeed = 800f;
    const float EnemyWidth = 14f;
    const float EnemyHeight = 14f;

    public float MoveSpeed = 50f;
    public float PatrolLeft = 0f;
    public float PatrolRight = 640f;

    float velocityX;
    float velocityY;
    bool facingRight = true;
    bool onGround;
    bool hitCeiling;

    public override void OnStart()
    {
        velocityX = MoveSpeed;
    }

    public override void OnUpdate(TimeContext time, InputContext input)
    {
        float dt = time.DeltaTime;

        // === Patrol ===
        if (_node.X >= PatrolRight) facingRight = false;
        if (_node.X <= PatrolLeft) facingRight = true;

        velocityX = facingRight ? MoveSpeed : -MoveSpeed;

        // === Gravity ===
        velocityY += Gravity * dt;
        if (velocityY > MaxFallSpeed) velocityY = MaxFallSpeed;

        // === Collision ===
        float px = _node.X - EnemyWidth / 2;
        float py = _node.Y - EnemyHeight / 2;
        float deltaX = velocityX * dt;
        float deltaY = velocityY * dt;

        int onGroundInt = 0;
        int hitCeilingInt = 0;

        var solidData = GameState.SolidTileData;
        int solidCount = GameState.SolidTileCount;

        unsafe
        {
            fixed (float* pSolid = solidData)
            {
                Interop.API.Physics2D_ResolvePlatform(
                    px, py, EnemyWidth, EnemyHeight,
                    &deltaX, &deltaY,
                    pSolid, solidCount,
                    &onGroundInt, &hitCeilingInt);
            }
        }

        onGround = onGroundInt != 0;
        hitCeiling = hitCeilingInt != 0;

        _node.X += deltaX;
        _node.Y += deltaY;

        // === Player contact damage ===
        Node player = GameState.PlayerNode;
        if (player.Handle != 0)
        {
            float px2 = _node.X;
            float py2 = _node.Y;
            float dx = px2 - player.X;
            float dy = py2 - player.Y;
            float overlapX = (EnemyWidth + 12f) / 2 - Mathf.Abs(dx);
            float overlapY = (EnemyHeight + 16f) / 2 - Mathf.Abs(dy);

            if (overlapX > 0 && overlapY > 0)
            {
                var pc = player.GetScript<PlayerController>();
                pc?.Respawn();
            }
        }
    }
}
