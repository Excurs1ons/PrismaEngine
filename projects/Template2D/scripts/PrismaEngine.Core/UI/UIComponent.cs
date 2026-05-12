using System;
using System.Collections.Generic;

namespace PrismaEngine.UI;

public abstract class UIComponent : Script
{
    // Position & Size
    public PrismaEngine.Vector2 AnchoredPosition { get; set; }
    public PrismaEngine.Vector2 Size { get; set; } = new PrismaEngine.Vector2(100, 100);
    
    // Anchor System (NGUI Style)
    public AnchorPresets AnchorPreset { get; set; } = AnchorPresets.MiddleCenter;
    public PrismaEngine.Vector2 AnchorMin { get; set; } = new PrismaEngine.Vector2(0.5f, 0.5f);
    public PrismaEngine.Vector2 AnchorMax { get; set; } = new PrismaEngine.Vector2(0.5f, 0.5f);
    public PrismaEngine.Vector2 Pivot { get; set; } = new PrismaEngine.Vector2(0.5f, 0.5f);
    public PrismaEngine.Vector2 OffsetMin { get; set; }
    public PrismaEngine.Vector2 OffsetMax { get; set; }
    
    // Computed Properties
    public PrismaEngine.Vector2 WorldPosition => CalculateWorldPosition();
    public PrismaEngine.Rect Rect => new PrismaEngine.Rect(
        WorldPosition.X - Size.X * Pivot.X,
        WorldPosition.Y - Size.Y * Pivot.Y,
        Size.X, Size.Y);
    
    // Render Properties
    public PrismaEngine.Color Color { get; set; } = PrismaEngine.Color.White;
    public int SortingOrder { get; set; } = 0;
    public bool IsRaycastTarget { get; set; } = true;
    public bool IsRaycastEnabled => IsRaycastTarget && Visible;
    
    // Hierarchy
    public Node? Parent { get; set; }
    public List<Node> Children { get; } = new();
    public int ChildCount => Children.Count;
    
    // Visibility
    public bool Visible { get; set; } = true;
    
    // Event Bubbling Control
    public bool IsStopBubbling { get; set; }
    
    // Internal state
    internal UIElementType ElementType { get; set; }
    internal bool _isInitialized;
    
    protected virtual PrismaEngine.Vector2 CalculateWorldPosition()
    {
        var pos = AnchoredPosition;
        
        // Apply parent offset if exists
        if (Parent != null)
        {
            var parentUI = Parent.GetScript<UIComponent>();
            if (parentUI != null)
            {
                pos.X += parentUI.AnchoredPosition.X;
                pos.Y += parentUI.AnchoredPosition.Y;
            }
        }
        
        return pos;
    }
    
    public override void OnCreate()
    {
        base.OnCreate();
        _isInitialized = true;
    }
    
    public override void OnUpdate(PrismaEngine.TimeContext time, PrismaEngine.InputContext input)
    {
        // UI update logic - can be overridden by subclasses
    }
    
    public virtual bool HitTest(PrismaEngine.Vector2 screenPos)
    {
        if (!IsRaycastEnabled) return false;
        return Rect.Contains(screenPos);
    }
    
    #region Event Callbacks
    
    public virtual void OnClick() { }
    public virtual void OnPointerEnter() { }
    public virtual void OnPointerExit() { }
    public virtual void OnPointerDown() { }
    public virtual void OnPointerUp() { }
    public virtual void OnBeginDrag(PrismaEngine.Vector2 position) { }
    public virtual void OnDrag(PrismaEngine.Vector2 position) { }
    public virtual void OnEndDrag() { }
    
    #endregion
    
    #region Hierarchy Management
    
    public void AddChild(Node child)
    {
        if (child == null) return;
        Children.Add(child);
        var childUI = child.GetScript<UIComponent>();
        if (childUI != null)
        {
            childUI.Parent = node;
        }
    }
    
    public void RemoveChild(Node child)
    {
        if (child == null) return;
        Children.Remove(child);
        var childUI = child.GetScript<UIComponent>();
        if (childUI != null)
        {
            childUI.Parent = null;
        }
    }
    
    public void BringToFront()
    {
        SortingOrder = Math.Max(SortingOrder + 1, 0);
    }
    
    public void SendToBack()
    {
        SortingOrder = Math.Min(SortingOrder - 1, 0);
    }
    
    #endregion
}