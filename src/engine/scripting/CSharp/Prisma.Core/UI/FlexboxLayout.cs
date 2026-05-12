using System;
using System.Collections.Generic;

namespace Prisma.UI;

public enum FlexDirection
{
    Row,
    RowReverse,
    Column,
    ColumnReverse
}

public enum FlexWrap
{
    NoWrap,
    Wrap,
    WrapReverse
}

public enum FlexJustify
{
    FlexStart,
    Center,
    FlexEnd,
    SpaceBetween,
    SpaceAround
}

public enum FlexAlign
{
    FlexStart,
    Center,
    FlexEnd,
    Stretch
}

[Serializable]
public partial class FlexboxLayout : UIComponent
{
    public FlexDirection Direction { get; set; } = FlexDirection.Row;
    public FlexWrap Wrap { get; set; } = FlexWrap.NoWrap;
    public FlexJustify JustifyContent { get; set; } = FlexJustify.FlexStart;
    public FlexAlign AlignItems { get; set; } = FlexAlign.Stretch;
    public FlexAlign AlignContent { get; set; } = FlexAlign.FlexStart;
    public Vector2 Spacing { get; set; }
    public Vector2 Padding { get; set; }
    public bool ChildForceExpandWidth { get; set; } = true;
    public bool ChildForceExpandHeight { get; set; } = true;
    
    private bool _isDirty = true;
    
    public FlexboxLayout()
    {
        ElementType = UIElementType.FlexboxLayout;
    }
    
    internal void SetDirty() => _isDirty = true;
    
    public override void OnUpdate(TimeContext time, InputContext input)
    {
        if (_isDirty)
        {
            CalculateLayout();
            _isDirty = false;
        }
    }
    
    internal void CalculateLayout()
    {
        var children = _children;
        if (children.Count == 0) return;
        
        var containerWidth = Size.X - Padding.X * 2;
        var containerHeight = Size.Y - Padding.Y * 2;
        
        float totalFixedWidth = 0f;
        float totalFixedHeight = 0f;
        int flexibleChildren = 0;
        
        foreach (var child in children)
        {
            var childUI = child.GetScript<UIComponent>();
            if (childUI == null) continue;
            
            if (Direction == FlexDirection.Row || Direction == FlexDirection.RowReverse)
            {
                totalFixedWidth += childUI.Size.X;
                flexibleChildren++;
            }
            else
            {
                totalFixedHeight += childUI.Size.Y;
            }
        }
        
        float spacingX = (Direction == FlexDirection.Row || Direction == FlexDirection.RowReverse) 
            ? Spacing.X * (children.Count - 1) : 0;
        float spacingY = (Direction == FlexDirection.Column || Direction == FlexDirection.ColumnReverse) 
            ? Spacing.Y * (children.Count - 1) : 0;
        
        float availableWidth = containerWidth - totalFixedWidth - spacingX;
        float availableHeight = containerHeight - totalFixedHeight - spacingY;
        
        float flexUnit = flexibleChildren > 0 ? availableWidth / flexibleChildren : 0;
        
        float x = Padding.X;
        float y = Padding.Y;
        
        float startX = Direction == FlexDirection.RowReverse ? containerWidth + Padding.X : Padding.X;
        
        foreach (var child in children)
        {
            var childUI = child.GetScript<UIComponent>();
            if (childUI == null) continue;
            
            float childX = Direction == FlexDirection.RowReverse ? startX - x : x;
            childUI.AnchoredPosition = new Vector2(childX, y);
            
            if (Direction == FlexDirection.Row || Direction == FlexDirection.RowReverse)
            {
                x += childUI.Size.X + Spacing.X;
            }
            else
            {
                y += childUI.Size.Y + Spacing.Y;
            }
        }
        
        ApplyJustify(containerWidth, totalFixedWidth, spacingX);
    }
    
    private void ApplyJustify(float containerWidth, float totalFixedWidth, float spacingX)
    {
        float totalContentWidth = totalFixedWidth + spacingX;
        float remainingSpace = containerWidth - totalContentWidth;
        
        float offset = 0;
        
        switch (JustifyContent)
        {
            case FlexJustify.Center:
                offset = remainingSpace / 2f;
                break;
            case FlexJustify.FlexEnd:
                offset = remainingSpace;
                break;
            case FlexJustify.SpaceBetween:
                if (_children.Count > 1)
                {
                    offset = 0;
                    float gap = remainingSpace / (_children.Count - 1);
                    for (int i = 0; i < _children.Count; i++)
                    {
                        var childUI = _children[i].GetScript<UIComponent>();
                        if (childUI != null)
                        {
                            childUI.AnchoredPosition = new Vector2(
                                childUI.AnchoredPosition.X + i * gap,
                                childUI.AnchoredPosition.Y);
                        }
                    }
                    return;
                }
                break;
            case FlexJustify.SpaceAround:
                if (_children.Count > 0)
                {
                    float gap = remainingSpace / _children.Count;
                    for (int i = 0; i < _children.Count; i++)
                    {
                        var childUI = _children[i].GetScript<UIComponent>();
                        if (childUI != null)
                        {
                            childUI.AnchoredPosition = new Vector2(
                                childUI.AnchoredPosition.X + (i + 0.5f) * gap,
                                childUI.AnchoredPosition.Y);
                        }
                    }
                    return;
                }
                break;
        }
        
        if (offset > 0)
        {
            foreach (var child in _children)
            {
                var childUI = child.GetScript<UIComponent>();
                if (childUI != null)
                {
                    childUI.AnchoredPosition = new Vector2(
                        childUI.AnchoredPosition.X + offset,
                        childUI.AnchoredPosition.Y);
                }
            }
        }
    }
}