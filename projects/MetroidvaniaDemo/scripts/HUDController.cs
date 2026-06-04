using System;
using System.Text;
using Prisma;

namespace GameScripts;

[Serializable]
public partial class HUDController : Script
{
    float hudUpdateTimer = 0f;
    string cachedHudText = "";

    public override void OnCreate()
    {
        cachedHudText = "HP: 5/5 | Dash: -- | DJump: --";
    }

    public override void OnUpdate(TimeContext time, InputContext input)
    {
        hudUpdateTimer -= time.DeltaTime;
        if (hudUpdateTimer <= 0f)
        {
            hudUpdateTimer = 0.1f;
            UpdateHudText();
        }

        unsafe
        {
            byte* textBytes = (byte*)System.Runtime.InteropServices.Marshal.StringToHGlobalAnsi(cachedHudText);
            Interop.API.UIDrawString(textBytes, 10f, 10f, 1.5f, 1f, 1f, 1f, 1f);
            System.Runtime.InteropServices.Marshal.FreeHGlobal((IntPtr)textBytes);
        }
    }

    void UpdateHudText()
    {
        int hp = GameState.CurrentHP;
        int maxHp = GameState.MaxHP;
        string hpStr = "HP: ";
        for (int i = 0; i < maxHp; i++)
            hpStr += (i < hp) ? "♥" : "♡";

        string dashStr = GameState.HasDash ? "Ready" : "locked";
        string djStr = GameState.HasDoubleJump ? "Ready" : "locked";

        cachedHudText = hpStr + " | Dash: " + dashStr + " | DJump: " + djStr;
    }
}
