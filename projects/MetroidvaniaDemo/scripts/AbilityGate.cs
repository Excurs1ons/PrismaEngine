using System;
using Prisma;

namespace GameScripts;

/// <summary>
/// Blocks the player's path until they have the required ability.
/// Destroys itself (opens gate) when the player proves they have it.
/// </summary>
[Serializable]
public partial class AbilityGate : Script
{
    public string RequiredAbility = "Dash";

    public override void OnUpdate(TimeContext time, InputContext input)
    {
        bool hasAbility = RequiredAbility switch
        {
            "Dash" => GameState.HasDash,
            _ => false
        };

        if (hasAbility)
        {
            Debug.Log("[AbilityGate] Gate opened! (ability: " + RequiredAbility + ")");
            node.Destroy();
        }
    }
}
