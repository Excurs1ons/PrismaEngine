using System;
using Prisma;

namespace GameScripts;

[Serializable]
public partial class PlayerController : Script
{
    const float MoveSpeed = 180f;
    const float JumpForce = 380f;
    const float Gravity = 980f;
    const float MaxFallSpeed = 800f;
    const float Friction = 0.85f;

    const float DashSpeed = 500f;
    const float DashDuration = 0.15f;
    const float DashCooldownTime = 0.6f;

    const float PlayerWidth = 12f;
    const float PlayerHeight = 16f;

    Vector2 velocity;
    float dashTimer;
    float dashCooldownTimer;
    bool facingRight = true;
    bool jumpHeld;
    bool onGround;
    bool hitCeiling;

    public override void OnCreate()
    {
        GameState.PlayerNode = node;
        _node.Position = new Vector2(64, 400);
    }

    public override void OnUpdate(TimeContext time, InputContext input)
    {
        float dt = time.DeltaTime;

        // === Input ===
        float moveX = 0;
        if (input.GetKey(KeyCode.D) || input.GetKey(KeyCode.Right)) moveX += 1;
        if (input.GetKey(KeyCode.A) || input.GetKey(KeyCode.Left)) moveX -= 1;

        bool jumpPressed = input.GetKey(KeyCode.W) || input.GetKey(KeyCode.Up) || input.GetKey(KeyCode.Space);
        bool dashPressed = input.GetKey(KeyCode.LShift);

        // === Jump ===
        if (jumpPressed && onGround && !jumpHeld)
        {
            velocity.Y = -JumpForce;
            onGround = false;
        }
        jumpHeld = jumpPressed;

        // === Dash ===
        dashTimer -= dt;
        dashCooldownTimer -= dt;

        if (GameState.HasDash && dashPressed && dashTimer <= 0 && dashCooldownTimer <= 0 && moveX != 0)
        {
            dashTimer = DashDuration;
            dashCooldownTimer = DashCooldownTime;
            velocity.X = (moveX > 0 ? 1 : -1) * DashSpeed;
            velocity.Y = 0;
        }

        // === Gravity ===
        if (dashTimer > 0)
        {
            velocity.Y = 0;
        }
        else
        {
            velocity.Y += Gravity * dt;
            if (velocity.Y > MaxFallSpeed) velocity.Y = MaxFallSpeed;
        }

        // === Horizontal movement ===
        if (dashTimer <= 0)
        {
            if (moveX != 0)
            {
                velocity.X = moveX * MoveSpeed;
                facingRight = moveX > 0;
            }
            else
            {
                velocity.X *= Friction;
                if (Mathf.Abs(velocity.X) < 1f) velocity.X = 0;
            }
        }

        // === Collision with tilemap ===
        float px = _node.X - PlayerWidth / 2;
        float py = _node.Y - PlayerHeight / 2;

        float deltaX = velocity.X * dt;
        float deltaY = velocity.Y * dt;

        int onGroundInt = 0;
        int hitCeilingInt = 0;

        var solidData = GameState.SolidTileData;
        int solidCount = GameState.SolidTileCount;

        unsafe
        {
            fixed (float* pSolid = solidData)
            {
                Interop.API.Physics2D_ResolvePlatform(
                    px, py, PlayerWidth, PlayerHeight,
                    &deltaX, &deltaY,
                    pSolid, solidCount,
                    &onGroundInt, &hitCeilingInt);
            }
        }

        onGround = onGroundInt != 0;
        hitCeiling = hitCeilingInt != 0;

        // Apply resolved movement
        _node.X += deltaX;
        _node.Y += deltaY;

        // === Fall death ===
        if (_node.Y > 520)
        {
            Respawn();
        }
    }

    public void Respawn()
    {
        _node.X = 64;
        _node.Y = 400;
        velocity = Vector2.Zero;
        onGround = false;
        dashTimer = 0;
        dashCooldownTimer = 0;
    }
}
