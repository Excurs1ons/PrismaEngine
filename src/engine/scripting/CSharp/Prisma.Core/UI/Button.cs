using System;

namespace Prisma.UI;

[Serializable]
public partial class Button : Image
{
    public ColorBlock Colors { get; set; } = ColorBlock.Default;
    public SpriteState SpriteState { get; set; }
    public Navigation Navigation { get; set; } = Navigation.Default;
    public bool IsInteractable { get; set; } = true;
    
    private ButtonState _currentState;
    
    public Button()
    {
        ElementType = UIElementType.Button;
    }
    
    public override void OnCreate()
    {
        base.OnCreate();
        _currentState = ButtonState.Normal;
        ApplyColorState();
    }
    
    private enum ButtonState
    {
        Normal,
        Highlighted,
        Pressed,
        Selected,
        Disabled
    }
    
    private void ApplyColorState()
    {
        if (!IsInteractable)
        {
            Color = Colors.DisabledColor * Colors.ColorMultiplier;
            return;
        }
        
        Color = _currentState switch
        {
            ButtonState.Normal => Colors.NormalColor * Colors.ColorMultiplier,
            ButtonState.Highlighted => Colors.HighlightedColor * Colors.ColorMultiplier,
            ButtonState.Pressed => Colors.PressedColor * Colors.ColorMultiplier,
            ButtonState.Selected => Colors.SelectedColor * Colors.ColorMultiplier,
            _ => Color
        };
    }
    
    internal void SetHighlighted(bool highlighted)
    {
        if (!IsInteractable) return;
        _currentState = highlighted ? ButtonState.Highlighted : ButtonState.Normal;
        ApplyColorState();
    }
    
    internal void SetPressed(bool pressed)
    {
        if (!IsInteractable) return;
        _currentState = pressed ? ButtonState.Pressed : ButtonState.Normal;
        ApplyColorState();
    }
    
    public override void OnClick()
    {
        if (!IsInteractable) return;
        
        _currentState = ButtonState.Pressed;
        ApplyColorState();
        
        OnButtonClick();
    }
    
    protected virtual void OnButtonClick()
    {
    }
}

[Serializable]
public struct Navigation
{
    public NavigationMode Mode { get; set; }
    
    public static Navigation Default => new() { Mode = NavigationMode.None };
    
    public static Navigation Automatic => new() { Mode = NavigationMode.Automatic };
}

public enum NavigationMode
{
    None,
    Automatic,
    Horizontal,
    Vertical,
    Explicit
}
