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

    const int MaxJumps = 2;
    const float DoubleJumpForce = 300f;

    // Movement feel tuning
    const float JumpBufferTime = 0.1f;
    const float CoyoteTime = 0.08f;
    const float Acceleration = 1200f;
    const float Deceleration = 1800f;
    const float FastFallMultiplier = 1.5f;

    Vector2 velocity;
    float dashTimer;
    float dashCooldownTimer;
    bool facingRight = true;
    bool jumpHeld;
    bool onGround;
    bool hitCeiling;
    int jumpCount;
    float jumpBufferTimer;
    float coyoteTimer;

    public override void OnCreate()
    {
        GameState.PlayerNode = node;
        _node.Position = new Vector2(64, 408);
        jumpCount = 0;
        jumpBufferTimer = 0f;
        coyoteTimer = 0f;
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

        // === Jump (buffer + coyote + variable height) ===
        jumpBufferTimer -= dt;
        coyoteTimer -= dt;

        if (jumpPressed && !jumpHeld)
            jumpBufferTimer = JumpBufferTime;

        if (onGround)
            coyoteTimer = CoyoteTime;

        bool canJump = (onGround || coyoteTimer > 0f) && jumpCount == 0;
        bool canDoubleJump = GameState.HasDoubleJump && jumpCount > 0 && jumpCount < MaxJumps && !onGround;

        if (jumpBufferTimer > 0f && !jumpHeld && (canJump || canDoubleJump))
        {
            float force = (jumpCount == 0) ? JumpForce : DoubleJumpForce;
            velocity.Y = -force;
            jumpCount++;
            onGround = false;
            coyoteTimer = 0f;
            jumpBufferTimer = 0f;
        }
        jumpHeld = jumpPressed;

        // Variable jump height: cut velocity if releasing key early
        if (!jumpPressed && velocity.Y < 0 && jumpCount > 0 && !onGround)
        {
            velocity.Y *= 0.5f;
        }

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
            float gravMultiplier = 1f;
            if ((input.GetKey(KeyCode.S) || input.GetKey(KeyCode.Down)) && velocity.Y > 0)
                gravMultiplier = FastFallMultiplier;
            velocity.Y += Gravity * gravMultiplier * dt;
            if (velocity.Y > MaxFallSpeed) velocity.Y = MaxFallSpeed;
        }

        // === Horizontal movement (acceleration model) ===
        if (dashTimer <= 0)
        {
            if (moveX != 0)
            {
                velocity.X += moveX * Acceleration * dt;
                if (Mathf.Abs(velocity.X) > MoveSpeed)
                    velocity.X = moveX * MoveSpeed;
                facingRight = moveX > 0;
            }
            else
            {
                if (velocity.X > 0)
                {
                    velocity.X -= Deceleration * dt;
                    if (velocity.X < 0) velocity.X = 0;
                }
                else if (velocity.X < 0)
                {
                    velocity.X += Deceleration * dt;
                    if (velocity.X > 0) velocity.X = 0;
                }
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

        if (onGroundInt != 0 && !onGround) {
            jumpCount = 0; // Reset jumps on landing
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
        _node.Y = 408;
        velocity = Vector2.Zero;
        onGround = false;
        jumpCount = 0;
        dashTimer = 0;
        dashCooldownTimer = 0;
    }

    public void OnAbilityUnlock(string ability)
    {
        Debug.Log("[PlayerController] Ability unlocked: " + ability);
    }
}
