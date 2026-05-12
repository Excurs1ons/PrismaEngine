using System;

namespace Prisma.UI;

public enum TransitionType
{
    None,
    ColorTint,
    SpriteSwap,
    Animation
}

public enum ButtonState
{
    Normal,
    Highlighted,
    Pressed,
    Disabled
}

public partial class Transition : UIComponent
{
    public TransitionType Type { get; set; } = TransitionType.ColorTint;
    public ColorBlock Colors { get; set; } = ColorBlock.Default;
    
    public Sprite? HighlightedSprite { get; set; }
    public Sprite? PressedSprite { get; set; }
    public Sprite? DisabledSprite { get; set; }
    
    private ButtonState _currentState = ButtonState.Normal;
    
    public Transition()
    {
    }
    
    internal void SetState(ButtonState state)
    {
        if (_currentState == state) return;
        
        _currentState = state;
        PlayTransition(state);
    }
    
    internal void PlayTransition(ButtonState state)
    {
        switch (Type)
        {
            case TransitionType.ColorTint:
                ApplyColorTransition(state);
                break;
            case TransitionType.SpriteSwap:
                ApplySpriteTransition(state);
                break;
            case TransitionType.Animation:
                break;
        }
    }
    
    private void ApplyColorTransition(ButtonState state)
    {
        var targetColor = state switch
        {
            ButtonState.Normal => Colors.NormalColor,
            ButtonState.Highlighted => Colors.HighlightedColor,
            ButtonState.Pressed => Colors.PressedColor,
            ButtonState.Disabled => Colors.DisabledColor,
            _ => Color.White
        };
        
        Color = targetColor * Colors.ColorMultiplier;
    }
    
    private void ApplySpriteTransition(ButtonState state)
    {
        var image = node.GetScript<Image>();
        if (image == null) return;
        
        switch (state)
        {
            case ButtonState.Normal:
                break;
            case ButtonState.Highlighted:
                if (HighlightedSprite != null) image.Sprite = HighlightedSprite;
                break;
            case ButtonState.Pressed:
                if (PressedSprite != null) image.Sprite = PressedSprite;
                break;
            case ButtonState.Disabled:
                if (DisabledSprite != null) image.Sprite = DisabledSprite;
                break;
        }
    }
}