using System;

namespace Prisma.UI;

public enum ScrollbarDirection
{
    Horizontal,
    Vertical,
    Both
}

[Serializable]
public partial class ScrollView : Panel
{
    public ScrollbarDirection ScrollDirection { get; set; } = ScrollbarDirection.Vertical;
    public bool HorizontalScrollbarEnabled { get; set; } = false;
    public bool VerticalScrollbarEnabled { get; set; } = true;
    public float ScrollSensitivity { get; set; } = 50f;
    public bool Inertia { get; set; } = true;
    public float DecelerationRate { get; set; } = 0.05f;
    public float MovementThreshold { get; set; } = 0.5f;
    
    private Vector2 _contentOffset;
    private Vector2 _velocity;
    private bool _isDragging;
    
    public Vector2 ContentOffset
    {
        get => _contentOffset;
        set
        {
            _contentOffset = value;
            UpdateContentPosition();
        }
    }
    
    public ScrollView()
    {
        ElementType = UIElementType.ScrollView;
    }
    
    private void UpdateContentPosition()
    {
        foreach (var child in _children)
        {
            var childUI = child.GetScript<UIComponent>();
            if (childUI != null)
            {
                childUI.AnchoredPosition = new Vector2(
                    childUI.AnchoredPosition.X + _contentOffset.X,
                    childUI.AnchoredPosition.Y + _contentOffset.Y);
            }
        }
    }
    
    internal void OnScroll(Vector2 scrollDelta)
    {
        float sensitivity = ScrollSensitivity * 0.01f;
        
        if (ScrollDirection == ScrollbarDirection.Vertical || ScrollDirection == ScrollbarDirection.Both)
        {
            _contentOffset.Y += scrollDelta.Y * sensitivity;
        }
        
        if (ScrollDirection == ScrollbarDirection.Horizontal || ScrollDirection == ScrollbarDirection.Both)
        {
            _contentOffset.X += scrollDelta.X * sensitivity;
        }
        
        UpdateContentPosition();
    }
    
    public override void OnBeginDrag(Vector2 position)
    {
        base.OnBeginDrag(position);
        _isDragging = true;
        _velocity = Vector2.Zero;
    }
    
    public override void OnDrag(Vector2 position)
    {
        if (!_isDragging) return;
        
        var delta = new Vector2(0, -position.Y * 0.1f);
        
        _velocity = delta;
        
        if (ScrollDirection == ScrollbarDirection.Vertical || ScrollDirection == ScrollbarDirection.Both)
        {
            _contentOffset.Y += delta.Y;
        }
        
        if (ScrollDirection == ScrollbarDirection.Horizontal || ScrollDirection == ScrollbarDirection.Both)
        {
            _contentOffset.X += delta.X;
        }
        
        UpdateContentPosition();
    }
    
    public override void OnEndDrag()
    {
        base.OnEndDrag();
        _isDragging = false;
        
        if (Inertia && MovementThreshold > 0)
        {
            ApplyInertia();
        }
    }
    
    private void ApplyInertia()
    {
        float magnitude = _velocity.Magnitude;
        
        if (magnitude > MovementThreshold)
        {
            float damping = 1f - DecelerationRate;
            
            _contentOffset.X += _velocity.X * damping;
            _contentOffset.Y += _velocity.Y * damping;
            
            UpdateContentPosition();
        }
    }
    
    public void ScrollToTop()
    {
        _contentOffset.Y = 0;
        UpdateContentPosition();
    }
    
    public void ScrollToBottom()
    {
        _contentOffset.Y = Size.Y;
        UpdateContentPosition();
    }
    
    public void ScrollToLeft()
    {
        _contentOffset.X = 0;
        UpdateContentPosition();
    }
    
    public void ScrollToRight()
    {
        _contentOffset.X = Size.X;
        UpdateContentPosition();
    }
}