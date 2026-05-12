using System;

namespace PrismaEngine.UI;

[Serializable]
public partial class Toggle : UIComponent
{
    public bool IsOn { get; set; } = false;
    public ColorBlock Colors { get; set; } = ColorBlock.Default;
    public Sprite? OnSprite { get; set; }
    public Sprite? OffSprite { get; set; }
    
    public event Action<bool>? OnValueChanged;
    
    public Toggle()
    {
        ElementType = UIElementType.Toggle;
    }
    
    public void SetIsOn(bool value, bool sendCallback = true)
    {
        if (IsOn == value) return;
        
        IsOn = value;
        UpdateVisual();
        
        if (sendCallback)
        {
            OnValueChanged?.Invoke(IsOn);
        }
    }
    
    public void ToggleValue()
    {
        SetIsOn(!IsOn);
    }
    
    private void UpdateVisual()
    {
        Color = IsOn ? Colors.NormalColor : Colors.DisabledColor;
        
        var image = node.GetScript<Image>();
        if (image != null)
        {
            image.Sprite = IsOn ? OnSprite : OffSprite;
        }
    }
    
    public override void OnClick()
    {
        base.OnClick();
        ToggleValue();
    }
}