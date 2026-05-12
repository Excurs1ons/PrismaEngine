using System;
using System.Collections.Generic;
using System.Linq;

namespace PrismaEngine.UI;

public sealed class UIBridge
{
    public static readonly UIBridge Instance = new();
    
    public InputPriorityMode PriorityMode { get; set; } = InputPriorityMode.UIFirst;
    
    private readonly List<Node> _uiRoots = new();
    private readonly Dictionary<int, Node> _hoveredElements = new();
    private readonly Dictionary<int, Node> _draggingElements = new();
    
    private UIBridge() { }
    
    public void RegisterUIRoot(Node root)
    {
        if (root.Handle != 0 && !_uiRoots.Contains(root))
        {
            _uiRoots.Add(root);
        }
    }
    
    public void UnregisterUIRoot(Node root)
    {
        _uiRoots.Remove(root);
    }
    
    public void ProcessPointerMove(int pointerId, Vector2 position)
    {
        var hit = RaycastUI(position, pointerId);
        
        if (_hoveredElements.TryGetValue(pointerId, out var previous) && previous.Handle != 0)
        {
            if (!previous.Equals(hit))
            {
                ProcessExit(previous, pointerId);
                
                if (hit.Handle != 0)
                {
                    ProcessEnter(hit, pointerId);
                }
            }
        }
        else if (hit.Handle != 0)
        {
            ProcessEnter(hit, pointerId);
        }
        
        _hoveredElements[pointerId] = hit;
    }
    
    public void ProcessPointerDown(int pointerId, Vector2 position)
    {
        var hit = RaycastUI(position, pointerId);
        if (hit.Handle == 0) return;
        
        _draggingElements[pointerId] = hit;
        
        BubbleEvent(hit, "OnPointerDown", position);
        
        EventBus.Publish(new PointerDownEvent 
        { 
            Target = hit, 
            ScreenPosition = position 
        });
    }
    
    public void ProcessPointerUp(int pointerId, Vector2 position)
    {
        var hit = RaycastUI(position, pointerId);
        
        if (_draggingElements.TryGetValue(pointerId, out var dragging) && dragging.Handle != 0)
        {
            if (_hoveredElements.TryGetValue(pointerId, out var hovered) && hovered.Equals(dragging))
            {
                BubbleEvent(dragging, "OnClick", position);
                EventBus.Publish(new ClickEvent 
                { 
                    Target = dragging, 
                    ScreenPosition = position 
                });
            }
            
            BubbleEvent(dragging, "OnEndDrag");
            _draggingElements[pointerId] = default;
        }
        
        if (hit.Handle != 0)
        {
            BubbleEvent(hit, "OnPointerUp", position);
        }
    }
    
    public void ProcessDrag(int pointerId, Vector2 position)
    {
        if (!_draggingElements.TryGetValue(pointerId, out var dragging) || dragging.Handle == 0)
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
    
    public Node RaycastUI(Vector2 screenPos, int pointerId = 0)
    {
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
        
        return default;
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
    
    private void BubbleEvent(Node target, string methodName, Vector2? position = null)
    {
        var current = target;
        while (current.Handle != 0)
        {
            var ui = current.GetScript<UIComponent>();
            if (ui == null) break;
            
            if (!ui.IsRaycastTarget) 
            {
                current = ui.Parent;
                continue;
            }
            
            InvokeUIEvent(ui, methodName, position);
            
            if (ui.IsStopBubbling) break;
            
            current = ui.Parent;
        }
    }
    
    private void InvokeUIEvent(UIComponent ui, string methodName, Vector2? position)
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
    UIFirst,
    LayerBased,
    GameFirst
}