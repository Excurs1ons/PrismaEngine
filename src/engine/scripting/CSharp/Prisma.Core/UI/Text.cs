using System;

namespace Prisma.UI;

[Serializable]
public partial class Text : UIComponent
{
    public string TextContent { get; set; } = "";
    public FontStyle FontStyle { get; set; } = FontStyle.Normal;
    public int FontSize { get; set; } = 14;
    public Color TextColor { get; set; } = Color.White;
    public TextAlignment Alignment { get; set; } = TextAlignment.MiddleCenter;
    public float LineSpacing { get; set; } = 1f;
    public bool RichText { get; set; } = true;
    public bool AutoSize { get; set; } = false;
    public Vector2 Margins { get; set; }
    
    private uint _fontId;
    
    public Vector2 PreferredSize { get; private set; }
    public int CachedTextHash { get; private set; }
    
    public Text()
    {
        ElementType = UIElementType.Text;
    }
    
    public override void OnCreate()
    {
        base.OnCreate();
        InvalidateCache();
    }
    
    public void SetText(string text)
    {
        if (TextContent == text) return;
        TextContent = text ?? "";
        InvalidateCache();
    }
    
    public void SetFont(uint fontId)
    {
        _fontId = fontId;
    }
    
    private void InvalidateCache()
    {
        CachedTextHash = 0;
        PreferredSize = Vector2.Zero;
    }
    
    protected override Vector2 CalculateWorldPosition()
    {
        var pos = base.CalculateWorldPosition();
        
        var textWidth = PreferredSize.X;
        var textHeight = PreferredSize.Y;
        
        if (Alignment == TextAlignment.TopCenter || 
            Alignment == TextAlignment.MiddleCenter || 
            Alignment == TextAlignment.BottomCenter)
        {
            pos.X -= textWidth / 2f;
        }
        else if (Alignment == TextAlignment.TopRight || 
                 Alignment == TextAlignment.MiddleRight || 
                 Alignment == TextAlignment.BottomRight)
        {
            pos.X -= textWidth;
        }
        
        if (Alignment == TextAlignment.TopLeft || 
            Alignment == TextAlignment.TopCenter || 
            Alignment == TextAlignment.TopRight)
        {
            pos.Y -= Size.Y - textHeight;
        }
        else if (Alignment == TextAlignment.MiddleLeft || 
                 Alignment == TextAlignment.MiddleCenter || 
                 Alignment == TextAlignment.MiddleRight)
        {
            pos.Y -= (Size.Y + textHeight) / 2f;
        }
        else
        {
            pos.Y -= textHeight;
        }
        
        return pos;
    }
    
    public override bool HitTest(Vector2 screenPos)
    {
        if (!IsRaycastEnabled) return false;
        return Rect.Contains(screenPos);
    }
}

[Serializable]
public enum FontStyle
{
    Normal = 0,
    Bold = 1,
    Italic = 2,
    BoldItalic = 3
}