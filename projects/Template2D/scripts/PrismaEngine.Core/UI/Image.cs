using System;

namespace PrismaEngine.UI;

/// <summary>
/// Image component for displaying sprites with various fill types.
/// 支持多种填充类型的图片组件。
/// </summary>
[Serializable]
public partial class Image : UIComponent
{
    public Sprite? Sprite { get; set; }
    public ImageType Type { get; set; } = ImageType.Simple;
    public bool FillCenter { get; set; } = true;
    public float FillAmount { get; set; } = 1f;
    public bool PreserveAspect { get; set; }
    
    // For sliced/tiled images
    public PrismaEngine.Vector4 Border { get; set; }
    
    // For filled images
    public FillMethod FillMethod { get; set; } = FillMethod.Horizontal;
    public bool FillClockwise { get; set; } = true;
    public float FillOrigin { get; set; } = 0f;
    
    public Image()
    {
        ElementType = UIElementType.Image;
        Size = Sprite?.Size ?? new PrismaEngine.Vector2(100, 100);
    }
    
    public override void OnCreate()
    {
        base.OnCreate();
        
        // Set default size from sprite if available
        if (Sprite != null)
        {
            Size = Sprite.Size;
        }
    }
    
    protected override PrismaEngine.Vector2 CalculateWorldPosition()
    {
        var pos = base.CalculateWorldPosition();
        
        // Apply anchor-based positioning for stretch anchors
        if (Parent != null)
        {
            var parentUI = Parent.GetScript<UIComponent>();
            if (parentUI != null)
            {
                var parentRect = parentUI.Rect;
                
                // Calculate based on anchor points
                var left = PrismaEngine.Mathf.Lerp(parentRect.xMin, parentRect.xMax, AnchorMin.X) + OffsetMin.X;
                var right = PrismaEngine.Mathf.Lerp(parentRect.xMin, parentRect.xMax, AnchorMax.X) + OffsetMax.X;
                var bottom = PrismaEngine.Mathf.Lerp(parentRect.yMin, parentRect.yMax, AnchorMin.Y) + OffsetMin.Y;
                var top = PrismaEngine.Mathf.Lerp(parentRect.yMin, parentRect.yMax, AnchorMax.Y) + OffsetMax.Y;
                
                if (Math.Abs(AnchorMin.X - AnchorMax.X) < 0.001f)
                {
                    // No horizontal stretch - use centered position
                    pos.X = (left + right) / 2f;
                }
                else
                {
                    // Horizontal stretch
                    pos.X = left;
                    Size = new PrismaEngine.Vector2(right - left, Size.Y);
                }
                
                if (Math.Abs(AnchorMin.Y - AnchorMax.Y) < 0.001f)
                {
                    // No vertical stretch - use centered position
                    pos.Y = (bottom + top) / 2f;
                }
                else
                {
                    // Vertical stretch
                    pos.Y = bottom;
                    Size = new PrismaEngine.Vector2(Size.X, top - bottom);
                }
            }
        }
        
        return pos;
    }
}

/// <summary>
/// Fill method for Image.FillMethod.
/// 图片填充方法。
/// </summary>
public enum FillMethod
{
    Horizontal,
    Vertical,
    Radial90,
    Radial180,
    Radial360
}
