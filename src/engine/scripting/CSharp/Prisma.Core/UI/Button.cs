using System;
using System.Text;

namespace Prisma.UI;

[Serializable]
public partial class Button : Image
{
    public ColorBlock Colors { get; set; } = ColorBlock.Default;
    public SpriteState SpriteState { get; set; }
    public Navigation Navigation { get; set; } = Navigation.Default;
    public bool IsInteractable { get; set; } = true;
    
    private ButtonState _currentState;
    
    public string ButtonText { get; set; } = "";

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

    public unsafe override void Render()
    {
        if (!Visible) return;

        var pos = WorldPosition;
        Interop.API.UIDrawQuad(pos.X, pos.Y, Size.X, Size.Y,
            Color.R, Color.G, Color.B, Color.A);

        if (!string.IsNullOrEmpty(ButtonText))
        {
            byte[] textBytes = Encoding.UTF8.GetBytes(ButtonText + "\0");
            fixed (byte* p = textBytes)
            {
                Interop.API.UIDrawString(p, pos.X + 4, pos.Y + 2, 1.0f,
                    1.0f, 1.0f, 1.0f, 1.0f);
            }
        }
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
