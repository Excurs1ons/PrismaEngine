using System;
using System.Collections.Generic;
using System.Linq;

namespace PrismaEngine.UI;

public static class UIBridge
{
    private static UIBridge? _instance;
    public static UIBridge Instance => _instance ??= new UIBridge();
    
    // Input priority mode
    public InputPriorityMode PriorityMode { get; set; } = InputPriorityMode.UIFirst;
    
    // All UI roots (Canvases)
    private readonly List<Node> _uiRoots = new();
    
    // Currently hovered element per pointer
    private readonly Dictionary<int, Node> _hoveredElements = new();
    
    // Currently dragging element per pointer
    private readonly Dictionary<int, Node> _draggingElements = new();
    
    private UIBridge() { }
    
    public void RegisterUIRoot(Node root)
    {
        if (!_uiRoots.Contains(root))
        {
            _uiRoots.Add(root);
        }
    }
    
    public void UnregisterUIRoot(Node root)
    {
        _uiRoots.Remove(root);
    }
    
    public void ProcessPointerMove(int pointerId, PrismaEngine.Vector2 position)
    {
        var hit = RaycastUI(position, pointerId);
        
        if (_hoveredElements.TryGetValue(pointerId, out var previous))
        {
            if (previous != hit)
            {
                // Pointer exited previous element
                ProcessExit(previous, pointerId);
                
                if (hit != null)
                {
                    // Pointer entered new element
                    ProcessEnter(hit, pointerId);
                }
            }
        }
        else if (hit != null)
        {
            ProcessEnter(hit, pointerId);
        }
        
        _hoveredElements[pointerId] = hit;
    }
    
    public void ProcessPointerDown(int pointerId, PrismaEngine.Vector2 position)
    {
        var hit = RaycastUI(position, pointerId);
        if (hit == null) return;
        
        // Store for drag detection
        _draggingElements[pointerId] = hit;
        
        // Bubble event (A)
        BubbleEvent(hit, "OnPointerDown", position);
        
        // EventBus publish (C)
        EventBus.Publish(new PointerDownEvent 
        { 
            Target = hit, 
            ScreenPosition = position 
        });
    }
    
    public void ProcessPointerUp(int pointerId, PrismaEngine.Vector2 position)
    {
        var hit = RaycastUI(position, pointerId);
        
        // If we were dragging, end the drag
        if (_draggingElements.TryGetValue(pointerId, out var dragging) && dragging != null)
        {
            if (_hoveredElements[pointerId] == dragging)
            {
                // Click on the same element
                BubbleEvent(dragging, "OnClick", position);
                EventBus.Publish(new ClickEvent 
                { 
                    Target = dragging, 
                    ScreenPosition = position 
                });
            }
            
            BubbleEvent(dragging, "OnEndDrag");
            _draggingElements[pointerId] = null!;
        }
        
        if (hit != null)
        {
            BubbleEvent(hit, "OnPointerUp", position);
        }
    }
    
    public void ProcessDrag(int pointerId, PrismaEngine.Vector2 position)
    {
        if (!_draggingElements.TryGetValue(pointerId, out var dragging) || dragging == null)
            return;
        
        BubbleEvent(dragging, "OnDrag", position);
    }
    
    private void ProcessEnter(Node target, int pointerId)
    {
        BubbleEvent(target, "OnPointerEnter");
        EventBus.Publish(new PointerEnterEvent { Target = target });
    }
    
    private void ProcessExit(Node target, int pointerId)
    {
        BubbleEvent(target, "OnPointerExit");
        EventBus.Publish(new PointerExitEvent { Target = target });
    }
    
    /// <summary>
    /// Raycast UI elements at screen position
    /// Returns the topmost UI element or null
    /// </summary>
    public Node? RaycastUI(PrismaEngine.Vector2 screenPos, int pointerId = 0)
    {
        // Sort all UI elements by SortingOrder descending
        var allElements = GetAllUIElements();
        allElements.Sort((a, b) => 
        {
            var aUI = a.GetScript<UIComponent>();
            var bUI = b.GetScript<UIComponent>();
            return bUI?.SortingOrder.CompareTo(aUI?.SortingOrder ?? 0) ?? 0;
        });
        
        foreach (var element in allElements)
        {
            var ui = element.GetScript<UIComponent>();
            if (ui?.HitTest(screenPos) == true)
            {
                return element;
            }
        }
        
        return null;
    }
    
    private List<Node> GetAllUIElements()
    {
        var elements = new List<Node>();
        
        foreach (var root in _uiRoots)
        {
            CollectUIElements(root, elements);
        }
        
        return elements;
    }
    
    private void CollectUIElements(Node node, List<Node> elements)
    {
        var ui = node.GetScript<UIComponent>();
        if (ui != null)
        {
            elements.Add(node);
        }
        
        foreach (var child in ui?.Children ?? Enumerable.Empty<Node>())
        {
            CollectUIElements(child, elements);
        }
    }
    
    /// <summary>
    /// Event bubbling - events bubble up from target to root (A)
    /// </summary>
    private void BubbleEvent(Node target, string methodName, PrismaEngine.Vector2? position = null)
    {
        var current = target;
        while (current != null)
        {
            var ui = current.GetScript<UIComponent>();
            if (ui == null) break;
            
            if (!ui.IsRaycastTarget) 
            {
                current = ui.Parent;
                continue;
            }
            
            // Call event method (B)
            InvokeUIEvent(ui, methodName, position);
            
            if (ui.IsStopBubbling) break;
            
            current = ui.Parent;
        }
    }
    
    private void InvokeUIEvent(UIComponent ui, string methodName, PrismaEngine.Vector2? position)
    {
        var method = typeof(UIComponent).GetMethod(methodName);
        if (method == null) return;
        
        if (position.HasValue)
        {
            if (methodName == "OnPointerDown" || methodName == "OnClick" || methodName == "OnDrag")
            {
                method.Invoke(ui, new object[] { position.Value });
                return;
            }
        }
        
        method.Invoke(ui, null);
    }
}

public enum InputPriorityMode
{
    UIFirst,      // A: UI always takes priority
    LayerBased,  // B: Based on camera layer order
    GameFirst    // C: Game objects take priority
}