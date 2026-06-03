using System;
using Prisma;

namespace GameScripts;

[Serializable]
public partial class PlayerHealth : Script
{
    const float InvincibilityDuration = 1.0f;
    const float BlinkInterval = 0.1f;

    float originalR, originalG, originalB;

    public override void OnCreate()
    {
        GameState.CurrentHP = GameState.MaxHP;
        GameState.InvincibleTimer = 0f;
        unsafe
        {
            uint idx = node.Handle & 0xFFFF;
            originalR = Interop.RenderData->ColorR[idx];
            originalG = Interop.RenderData->ColorG[idx];
            originalB = Interop.RenderData->ColorB[idx];
        }
    }

    public void TakeDamage(int amount)
    {
        if (GameState.InvincibleTimer > 0f) return;

        GameState.CurrentHP -= amount;
        GameState.InvincibleTimer = InvincibilityDuration;

        Debug.Log("[PlayerHealth] Took " + amount + " damage! HP: " + GameState.CurrentHP + "/" + GameState.MaxHP);

        if (GameState.CurrentHP <= 0)
        {
            Die();
        }
    }

    public void Heal(int amount)
    {
        GameState.CurrentHP += amount;
        if (GameState.CurrentHP > GameState.MaxHP)
            GameState.CurrentHP = GameState.MaxHP;
    }

    void Die()
    {
        Debug.Log("[PlayerHealth] Player died!");
        GameState.CurrentHP = GameState.MaxHP;
        GameState.InvincibleTimer = 0f;

        var pc = node.GetScript<PlayerController>();
        pc?.Respawn();

        // Reset color after death
        unsafe
        {
            uint idx = node.Handle & 0xFFFF;
            Interop.RenderData->ColorR[idx] = originalR;
            Interop.RenderData->ColorG[idx] = originalG;
            Interop.RenderData->ColorB[idx] = originalB;
        }
    }

    public override void OnUpdate(TimeContext time, InputContext input)
    {
        if (GameState.InvincibleTimer > 0f)
        {
            GameState.InvincibleTimer -= time.DeltaTime;

            // Blink effect: alternate between red and original color
            unsafe
            {
                uint idx = node.Handle & 0xFFFF;
                bool isRed = (Mathf.Floor(GameState.InvincibleTimer / BlinkInterval) % 2) < 1;
                if (isRed)
                {
                    Interop.RenderData->ColorR[idx] = 1.0f;
                    Interop.RenderData->ColorG[idx] = 0.2f;
                    Interop.RenderData->ColorB[idx] = 0.2f;
                }
                else
                {
                    Interop.RenderData->ColorR[idx] = originalR;
                    Interop.RenderData->ColorG[idx] = originalG;
                    Interop.RenderData->ColorB[idx] = originalB;
                }
            }
        }
    }
}
