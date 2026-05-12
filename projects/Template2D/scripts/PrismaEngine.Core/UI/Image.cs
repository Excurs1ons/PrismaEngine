using System;

namespace PrismaEngine.UI;

[Serializable]
public partial class Image : UIComponent
{
    public Sprite? Sprite { get; set; }
    public ImageType Type { get; set; } = ImageType.Simple;
    public bool FillCenter { get; set; } = true;
    public float FillAmount { get; set; } = 1f;
    public bool PreserveAspect { get; set; }
    
    public Vector4 Border { get; set; }
    
    public FillMethod FillMethod { get; set; } = FillMethod.Horizontal;
    public bool FillClockwise { get; set; } = true;
    public float FillOrigin { get; set; } = 0f;
    
    public Image()
    {
        ElementType = UIElementType.Image;
        Size = Sprite?.Size ?? new Vector2(100, 100);
    }
    
    public override void OnCreate()
    {
        base.OnCreate();
        
        if (Sprite != null)
        {
            Size = Sprite.Size;
        }
    }
    
    protected override Vector2 CalculateWorldPosition()
    {
        var pos = base.CalculateWorldPosition();
        
        if (HasParent)
        {
            var parentUI = Parent.GetScript<UIComponent>();
            if (parentUI != null)
            {
                var parentRect = parentUI.Rect;
                
                var left = Mathf.Lerp(parentRect.xMin, parentRect.xMax, AnchorMin.X) + OffsetMin.X;
                var right = Mathf.Lerp(parentRect.xMin, parentRect.xMax, AnchorMax.X) + OffsetMax.X;
                var bottom = Mathf.Lerp(parentRect.yMin, parentRect.yMax, AnchorMin.Y) + OffsetMin.Y;
                var top = Mathf.Lerp(parentRect.yMin, parentRect.yMax, AnchorMax.Y) + OffsetMax.Y;
                
                if (Math.Abs(AnchorMin.X - AnchorMax.X) < 0.001f)
                {
                    pos.X = (left + right) / 2f;
                }
                else
                {
                    pos.X = left;
                    Size = new Vector2(right - left, Size.Y);
                }
                
                if (Math.Abs(AnchorMin.Y - AnchorMax.Y) < 0.001f)
                {
                    pos.Y = (bottom + top) / 2f;
                }
                else
                {
                    pos.Y = bottom;
                    Size = new Vector2(Size.X, top - bottom);
                }
            }
        }
        
        return pos;
    }
}

public enum FillMethod
{
    Horizontal,
    Vertical,
    Radial90,
    Radial180,
    Radial360
}