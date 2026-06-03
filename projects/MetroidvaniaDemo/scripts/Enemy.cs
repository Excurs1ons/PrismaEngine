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

    const float DetectionRange = 120f;
    const float ChaseSpeed = 80f;
    const int ContactDamage = 1;
    const float KnockbackForceX = 120f;
    const float KnockbackForceY = -200f;
    const float AttackCooldown = 0.5f;

    float velocityX;
    float velocityY;
    bool facingRight = true;
    bool onGround;
    bool hitCeiling;
    float attackCooldownTimer;

    public override void OnStart()
    {
        velocityX = MoveSpeed;
    }

    public override void OnUpdate(TimeContext time, InputContext input)
    {
        float dt = time.DeltaTime;
        attackCooldownTimer -= dt;

        Node player = GameState.PlayerNode;

        // === AI State ===
        if (player.Handle != 0)
        {
            float dist = Mathf.Abs(_node.X - player.X);

            if (dist < DetectionRange)
            {
                float dir = (player.X > _node.X) ? 1f : -1f;
                velocityX = dir * ChaseSpeed;
                facingRight = dir > 0;
            }
            else if (dist > DetectionRange * 1.5f)
            {
                if (_node.X >= PatrolRight) facingRight = false;
                if (_node.X <= PatrolLeft) facingRight = true;
                velocityX = facingRight ? MoveSpeed : -MoveSpeed;
            }
            // else: buffer zone — maintain current direction
        }
        else
        {
            if (_node.X >= PatrolRight) facingRight = false;
            if (_node.X <= PatrolLeft) facingRight = true;
            velocityX = facingRight ? MoveSpeed : -MoveSpeed;
        }

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
        if (player.Handle != 0 && attackCooldownTimer <= 0)
        {
            float dx = _node.X - player.X;
            float dy = _node.Y - player.Y;
            float overlapX = (EnemyWidth + 12f) / 2 - Mathf.Abs(dx);
            float overlapY = (EnemyHeight + 16f) / 2 - Mathf.Abs(dy);

            if (overlapX > 0 && overlapY > 0)
            {
                if (dy < -4f && onGround)
                {
                    // Player stomp — recoil enemy downward
                    var pc = player.GetScript<PlayerController>();
                    velocityX = (dx > 0 ? 1f : -1f) * KnockbackForceX;
                    velocityY = KnockbackForceY * 0.5f;
                }
                else
                {
                    var ph = player.GetScript<PlayerHealth>();
                    ph?.TakeDamage(ContactDamage);

                    float kbDir = (dx > 0 ? 1f : -1f);
                    unsafe
                    {
                        uint pIdx = player.Handle & 0xFFFF;
                        // Push player position as a one-time knockback impulse
                        player.X += kbDir * KnockbackForceX * dt;
                    }
                }

                attackCooldownTimer = AttackCooldown;
            }
        }
    }
}
