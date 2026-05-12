using System;

namespace Prisma.UI;

public enum SliderDirection
{
    LeftToRight,
    RightToLeft,
    BottomToTop,
    TopToBottom
}

[Serializable]
public partial class Slider : UIComponent
{
    public float MinValue { get; set; } = 0f;
    public float MaxValue { get; set; } = 1f;
    public float Value { get; set; } = 0.5f;
    public bool WholeNumbers { get; set; } = false;
    public SliderDirection Direction { get; set; } = SliderDirection.LeftToRight;
    
    public Image? Background { get; set; }
    public Image? FillImage { get; set; }
    public Image? HandleImage { get; set; }
    
    public event Action<float>? OnValueChanged;
    
    public Slider()
    {
        ElementType = UIElementType.Slider;
    }
    
    public void SetValue(float value, bool sendCallback = true)
    {
        float newValue = Math.Clamp(value, MinValue, MaxValue);
        
        if (WholeNumbers)
        {
            newValue = (float)Math.Round(newValue);
        }
        
        if (Math.Abs(Value - newValue) < 0.0001f) return;
        
        Value = newValue;
        UpdateVisual();
        
        if (sendCallback)
        {
            OnValueChanged?.Invoke(Value);
        }
    }
    
    private void UpdateVisual()
    {
        float normalized = (Value - MinValue) / (MaxValue - MinValue);
        
        if (FillImage != null)
        {
            FillImage.FillAmount = normalized;
        }
        
        if (HandleImage != null)
        {
            float handlePos = normalized * Size.X;
            HandleImage.AnchoredPosition = new Vector2(handlePos, HandleImage.AnchoredPosition.Y);
        }
    }
}