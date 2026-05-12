using System;
using System.Collections.Generic;

namespace PrismaEngine.UI;

public abstract partial class UIComponent : Script
{
    public Vector2 AnchoredPosition { get; set; }
    public Vector2 Size { get; set; } = new Vector2(100, 100);
    
    public AnchorPresets AnchorPreset { get; set; } = AnchorPresets.MiddleCenter;
    public Vector2 AnchorMin { get; set; } = new Vector2(0.5f, 0.5f);
    public Vector2 AnchorMax { get; set; } = new Vector2(0.5f, 0.5f);
    public Vector2 Pivot { get; set; } = new Vector2(0.5f, 0.5f);
    public Vector2 OffsetMin { get; set; }
    public Vector2 OffsetMax { get; set; }
    
    public Vector2 WorldPosition => CalculateWorldPosition();
    public Rect Rect => new Rect(
        WorldPosition.X - Size.X * Pivot.X,
        WorldPosition.Y - Size.Y * Pivot.Y,
        Size.X, Size.Y);
    
    public Color Color { get; set; } = Color.White;
    public int SortingOrder { get; set; } = 0;
    public bool IsRaycastTarget { get; set; } = true;
    public bool IsRaycastEnabled => IsRaycastTarget && Visible;
    
    internal Node _parent;
    public Node Parent 
    { 
        get => _parent; 
        set => _parent = value; 
    }
    public bool HasParent => _parent.Handle != 0;
    
    internal List<Node> _children = new();
    public List<Node> Children => _children;
    public int ChildCount => _children.Count;
    
    public bool Visible { get; set; } = true;
    public bool IsStopBubbling { get; set; }
    
    internal UIElementType ElementType { get; set; }
    internal bool _isInitialized;
    
    protected virtual Vector2 CalculateWorldPosition()
    {
        var pos = AnchoredPosition;
        
        if (HasParent)
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
    
    public override void OnUpdate(TimeContext time, InputContext input)
    {
    }
    
    public virtual bool HitTest(Vector2 screenPos)
    {
        if (!IsRaycastEnabled) return false;
        return Rect.Contains(screenPos);
    }
    
    public virtual void OnClick() { }
    public virtual void OnPointerEnter() { }
    public virtual void OnPointerExit() { }
    public virtual void OnPointerDown() { }
    public virtual void OnPointerUp() { }
    public virtual void OnBeginDrag(Vector2 position) { }
    public virtual void OnDrag(Vector2 position) { }
    public virtual void OnEndDrag() { }
    
    public void AddChild(Node child)
    {
        if (child.Handle == 0) return;
        _children.Add(child);
        var childUI = child.GetScript<UIComponent>();
        if (childUI != null)
        {
            childUI._parent = node;
        }
    }
    
    public void RemoveChild(Node child)
    {
        if (child.Handle == 0) return;
        _children.Remove(child);
        var childUI = child.GetScript<UIComponent>();
        if (childUI != null)
        {
            childUI._parent = default;
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
}