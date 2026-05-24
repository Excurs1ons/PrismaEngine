# C# UI System - Phase 1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement UIComponent base class + Image + Button + Text components for PrismaEngine C# scripting system

**Architecture:** Extend existing Script system with UI-specific base class. UI elements use existing SoA RenderBuffer with new UI fields. Follow NGUI-style anchor/pivot system. Support event bubbling, Script callbacks, and optional EventBus.

**Tech Stack:** C# 10+, existing PrismaEngine.Core, existing PrismaScriptGenerator

---

## File Structure

```
PrismaEngine.Core/
├── UI/
│   ├── UIComponent.cs       # Base class (NEW)
│   ├── Image.cs             # Image component (NEW)
│   ├── Button.cs            # Button component (NEW)
│   ├── Text.cs               # Text component (NEW)
│   ├── UIBridge.cs           # Event handling (NEW)
│   ├── AnchorPresets.cs     # Anchor enums (NEW)
│   ├── ColorBlock.cs         # Color block struct (NEW)
│   ├── Sprite.cs             # Sprite reference (NEW)
│   └── UIElementType.cs      # Element type enum (NEW)
└── PrismaEngine.Core.csproj  # Modify: add UI/ folder
```

---

## Task 1: Create UI Infrastructure Files

**Files:**
- Create: `projects/Prisma2D/scripts/PrismaEngine.Core/UI/UIElementType.cs`
- Create: `projects/Prisma2D/scripts/PrismaEngine.Core/UI/AnchorPresets.cs`
- Create: `projects/Prisma2D/scripts/PrismaEngine.Core/UI/ColorBlock.cs`

- [ ] **Step 1: Create UIElementType.cs**

```csharp
namespace PrismaEngine;

public enum UIElementType
{
    Image = 0,
    Button = 1,
    Text = 2,
    Panel = 3,
    ScrollView = 4,
    Slider = 5,
    Toggle = 6,
    Dropdown = 7,
    TextField = 8,
    ContextMenu = 9
}
```

- [ ] **Step 2: Create AnchorPresets.cs**

```csharp
using System;

namespace PrismaEngine.UI;

[Flags]
public enum AnchorPresets
{
    None = 0,
    TopLeft = 1 << 0,
    TopCenter = 1 << 1,
    TopRight = 1 << 2,
    MiddleLeft = 1 << 3,
    MiddleCenter = 1 << 4,
    MiddleRight = 1 << 5,
    BottomLeft = 1 << 6,
    BottomCenter = 1 << 7,
    BottomRight = 1 << 8,
    StretchHorizontal = 1 << 9,
    StretchVertical = 1 << 10,
    StretchAll = StretchHorizontal | StretchVertical,
    Custom = 1 << 11
}

public enum TextAlignment
{
    TopLeft, TopCenter, TopRight,
    MiddleLeft, MiddleCenter, MiddleRight,
    BottomLeft, BottomCenter, BottomRight
}

public enum ImageType
{
    Simple,
    Sliced,
    Tiled,
    Filled
}
```

- [ ] **Step 3: Create ColorBlock.cs**

```csharp
using System;
using UnityEngine;

namespace PrismaEngine.UI;

[Serializable]
public struct ColorBlock
{
    public Color NormalColor;
    public Color HighlightedColor;
    public Color PressedColor;
    public Color SelectedColor;
    public Color DisabledColor;
    public float ColorMultiplier;
    
    public static ColorBlock Default => new()
    {
        NormalColor = Color.White,
        HighlightedColor = new Color(0.78f, 0.78f, 0.78f),
        PressedColor = new Color(0.55f, 0.55f, 0.55f),
        SelectedColor = new Color(0.68f, 0.68f, 0.68f),
        DisabledColor = new Color(0.5f, 0.5f, 0.5f, 0.5f),
        ColorMultiplier = 1f
    };
}

[Serializable]
public struct SpriteState
{
    public Color HighlightedSprite;
    public Color PressedSprite;
    public Color SelectedSprite;
    public Color DisabledSprite;
}
```

- [ ] **Step 4: Create Sprite.cs**

```csharp
using System;

namespace PrismaEngine.UI;

public class Sprite
{
    public uint Id { get; }
    public string Name { get; }
    public Rect Rect { get; }
    public Vector2 Pivot { get; }
    public Vector2 Size { get; }
    
    internal Sprite(uint id, string name, Rect rect, Vector2 pivot)
    {
        Id = id;
        Name = name;
        Rect = rect;
        Pivot = pivot;
        Size = new Vector2(rect.Width, rect.Height);
    }
}
```

- [ ] **Step 5: Commit**

```bash
git add projects/Prisma2D/scripts/PrismaEngine.Core/UI/
git commit -m "feat(ui): add UI infrastructure files (UIElementType, AnchorPresets, ColorBlock, Sprite)"
```

---

## Task 2: Create UIComponent Base Class

**Files:**
- Create: `projects/Prisma2D/scripts/PrismaEngine.Core/UI/UIComponent.cs`

- [ ] **Step 1: Create UIComponent.cs**

```csharp
using System;
using System.Collections.Generic;
using System.Runtime.CompilerServices;

namespace PrismaEngine.UI;

public abstract class UIComponent : Script
{
    // Position & Size
    public Vector2 AnchoredPosition { get; set; }
    public Vector2 Size { get; set; } = new Vector2(100, 100);
    
    // Anchor System (NGUI Style)
    public AnchorPresets AnchorPreset { get; set; } = AnchorPresets.MiddleCenter;
    public Vector2 AnchorMin { get; set; } = new Vector2(0.5f, 0.5f);
    public Vector2 AnchorMax { get; set; } = new Vector2(0.5f, 0.5f);
    public Vector2 Pivot { get; set; } = new Vector2(0.5f, 0.5f);
    public Vector2 OffsetMin { get; set; }
    public Vector2 OffsetMax { get; set; }
    
    // Computed Properties
    public Vector2 WorldPosition => CalculateWorldPosition();
    public Rect Rect => new Rect(WorldPosition - Size * Pivot, Size);
    
    // Render Properties
    public Color Color { get; set; } = Color.White;
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
    
    protected virtual Vector2 CalculateWorldPosition()
    {
        var pos = AnchoredPosition;
        
        // Apply parent offset if exists
        if (Parent != null)
        {
            var parentUI = Parent.GetScript<UIComponent>();
            if (parentUI != null)
            {
                pos += parentUI.AnchoredPosition;
            }
        }
        
        return pos;
    }
    
    public override void OnCreate()
    {
        _isInitialized = true;
    }
    
    public override void OnStart()
    {
        // Initialize UI state
    }
    
    public override void OnUpdate(PrismaEngine.TimeContext time, PrismaEngine.InputContext input)
    {
        // UI update logic - can be overridden by subclasses
    }
    
    public virtual bool HitTest(Vector2 screenPos)
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
    public virtual void OnBeginDrag(Vector2 position) { }
    public virtual void OnDrag(Vector2 position) { }
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
```

- [ ] **Step 2: Commit**

```bash
git add projects/Prisma2D/scripts/PrismaEngine.Core/UI/UIComponent.cs
git commit -m "feat(ui): add UIComponent base class with anchor system and event callbacks"
```

---

## Task 3: Create Image Component

**Files:**
- Create: `projects/Prisma2D/scripts/PrismaEngine.Core/UI/Image.cs`

- [ ] **Step 1: Create Image.cs**

```csharp
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
    
    // For sliced/tiled images
    public Vector4 Border { get; set; }
    
    // For filled images
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
        
        // Set default size from sprite if available
        if (Sprite != null)
        {
            Size = Sprite.Size;
        }
    }
    
    protected override Vector2 CalculateWorldPosition()
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
                var left = Mathf.Lerp(parentRect.xMin, parentRect.xMax, AnchorMin.X) + OffsetMin.X;
                var right = Mathf.Lerp(parentRect.xMin, parentRect.xMax, AnchorMax.X) + OffsetMax.X;
                var bottom = Mathf.Lerp(parentRect.yMin, parentRect.yMax, AnchorMin.Y) + OffsetMin.Y;
                var top = Mathf.Lerp(parentRect.yMin, parentRect.yMax, AnchorMax.Y) + OffsetMax.Y;
                
                if (Math.Abs(AnchorMin.X - AnchorMax.X) < 0.001f)
                {
                    // No horizontal stretch - use centered position
                    pos.X = (left + right) / 2f;
                }
                
                if (Math.Abs(AnchorMin.Y - AnchorMax.Y) < 0.001f)
                {
                    // No vertical stretch - use centered position
                    pos.Y = (bottom + top) / 2f;
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
```

- [ ] **Step 2: Commit**

```bash
git add projects/Prisma2D/scripts/PrismaEngine.Core/UI/Image.cs
git commit -m "feat(ui): add Image component with ImageType support (Simple, Sliced, Tiled, Filled)"
```

---

## Task 4: Create Button Component

**Files:**
- Create: `projects/Prisma2D/scripts/PrismaEngine.Core/UI/Button.cs`

- [ ] **Step 1: Create Button.cs**

```csharp
using System;

namespace PrismaEngine.UI;

[Serializable]
public partial class Button : Image
{
    public ColorBlock Colors { get; set; } = ColorBlock.Default;
    public SpriteState SpriteState { get; set; }
    public Navigation Navigation { get; set; } = Navigation.Default;
    public bool IsInteractable { get; set; } = true;
    
    // Transition state
    private ButtonState _currentState;
    
    public Button()
    {
        ElementType = UIElementType.Button;
    }
    
    public override void OnCreate()
    {
        base.OnCreate();
        _currentState = ButtonState.Normal;
        ApplyColorState();
    }
    
    private enum ButtonState
    {
        Normal,
        Highlighted,
        Pressed,
        Selected,
        Disabled
    }
    
    private void ApplyColorState()
    {
        if (!IsInteractable)
        {
            Color = Colors.DisabledColor * Colors.ColorMultiplier;
            return;
        }
        
        Color = _currentState switch
        {
            ButtonState.Normal => Colors.NormalColor * Colors.ColorMultiplier,
            ButtonState.Highlighted => Colors.HighlightedColor * Colors.ColorMultiplier,
            ButtonState.Pressed => Colors.PressedColor * Colors.ColorMultiplier,
            ButtonState.Selected => Colors.SelectedColor * Colors.ColorMultiplier,
            _ => Color
        };
    }
    
    // Event handlers - called by UIBridge
    internal void SetHighlighted(bool highlighted)
    {
        if (!IsInteractable) return;
        _currentState = highlighted ? ButtonState.Highlighted : ButtonState.Normal;
        ApplyColorState();
    }
    
    internal void SetPressed(bool pressed)
    {
        if (!IsInteractable) return;
        _currentState = pressed ? ButtonState.Pressed : ButtonState.Normal;
        ApplyColorState();
    }
    
    public override void OnClick()
    {
        if (!IsInteractable) return;
        
        // Transition to pressed state briefly
        _currentState = ButtonState.Pressed;
        ApplyColorState();
        
        // User can override this
        OnButtonClick();
    }
    
    protected virtual void OnButtonClick()
    {
        // User implementation
    }
}

[Serializable]
public struct Navigation
{
    public NavigationMode Mode { get; set; }
    
    public static Navigation Default => new() { Mode = NavigationMode.None };
    
    public static Navigation Automatic => new() { Mode = NavigationMode.Automatic };
}

public enum NavigationMode
{
    None,
    Automatic,
    Horizontal,
    Vertical,
    Explicit
}
```

- [ ] **Step 2: Commit**

```bash
git add projects/Prisma2D/scripts/PrismaEngine.Core/UI/Button.cs
git commit -m "feat(ui): add Button component with ColorBlock state transitions"
```

---

## Task 5: Create Text Component

**Files:**
- Create: `projects/Prisma2D/scripts/PrismaEngine.Core/UI/Text.cs`

- [ ] **Step 1: Create Text.cs**

```csharp
using System;
using System.Text;

namespace PrismaEngine.UI;

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
    
    // Font reference
    private uint _fontId;
    
    // Computed text dimensions
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
        CachedTextHash = 0; // Forces recalculation
        PreferredSize = Vector2.Zero;
    }
    
    protected override Vector2 CalculateWorldPosition()
    {
        var pos = base.CalculateWorldPosition();
        
        // Text alignment adjustments
        var textWidth = PreferredSize.X;
        var textHeight = PreferredSize.Y;
        
        // Horizontal alignment
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
        
        // Vertical alignment
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
        // Text doesn't block raycasts by default (transparent areas)
        if (!IsRaycastEnabled) return false;
        
        // For text, we might want more precise hit testing
        // This is a simplified version
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
```

- [ ] **Step 2: Commit**

```bash
git add projects/Prisma2D/scripts/PrismaEngine.Core/UI/Text.cs
git commit -m "feat(ui): add Text component with alignment and rich text support"
```

---

## Task 6: Create UIBridge (Event Handling)

**Files:**
- Create: `projects/Prisma2D/scripts/PrismaEngine.Core/UI/UIBridge.cs`
- Create: `projects/Prisma2D/scripts/PrismaEngine.Core/UI/EventBus.cs`

- [ ] **Step 1: Create EventBus.cs**

```csharp
using System;
using System.Collections.Generic;
using System本部;

namespace PrismaEngine.UI;

#region Event Types

public abstract class UIEvent
{
    public DateTime Timestamp { get; set; } = DateTime.Now;
}

public class ClickEvent : UIEvent
{
    public Node Target { get; set; } = default;
    public Vector2 ScreenPosition { get; set; }
}

public class PointerEnterEvent : UIEvent
{
    public Node Target { get; set; } = default;
}

public class PointerExitEvent : UIEvent
{
    public Node Target { get; set; } = default;
}

public class PointerDownEvent : UIEvent
{
    public Node Target { get; set; } = default;
    public Vector2 ScreenPosition { get; set; }
}

public class DragEvent : UIEvent
{
    public Node Target { get; set; } = default;
    public Vector2 ScreenPosition { get; set; }
}

#endregion

public static class EventBus
{
    private static readonly Dictionary<Type, List<Delegate>> _handlers = new();
    private static readonly object _lock = new();
    
    public static void Subscribe<T>(Action<T> handler) where T : UIEvent
    {
        lock (_lock)
        {
            var type = typeof(T);
            if (!_handlers.TryGetValue(type, out var handlers))
            {
                handlers = new List<Delegate>();
                _handlers[type] = handlers;
            }
            handlers.Add(handler);
        }
    }
    
    public static void Unsubscribe<T>(Action<T> handler) where T : UIEvent
    {
        lock (_lock)
        {
            var type = typeof(T);
            if (_handlers.TryGetValue(type, out var handlers))
            {
                handlers.Remove(handler);
            }
        }
    }
    
    public static void Publish<T>(T eventData) where T : UIEvent
    {
        lock (_lock)
        {
            if (_handlers.TryGetValue(typeof(T), out var handlers))
            {
                foreach (var handler in handlers.ToArray())
                {
                    try
                    {
                        ((Action<T>)handler)(eventData);
                    }
                    catch (Exception ex)
                    {
                        Debug.LogError($"Event handler error: {ex.Message}");
                    }
                }
            }
        }
    }
    
    public static void Clear()
    {
        lock (_lock)
        {
            _handlers.Clear();
        }
    }
}
```

- [ ] **Step 2: Create UIBridge.cs**

```csharp
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
    
    public void ProcessPointerMove(int pointerId, Vector2 position)
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
    
    public void ProcessPointerDown(int pointerId, Vector2 position)
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
    
    public void ProcessPointerUp(int pointerId, Vector2 position)
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
    
    public void ProcessDrag(int pointerId, Vector2 position)
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
    public Node? RaycastUI(Vector2 screenPos, int pointerId = 0)
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
    private void BubbleEvent(Node target, string methodName, Vector2? position = null)
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
    UIFirst,      // A: UI always takes priority
    LayerBased,  // B: Based on camera layer order
    GameFirst    // C: Game objects take priority
}
```

- [ ] **Step 3: Commit**

```bash
git add projects/Prisma2D/scripts/PrismaEngine.Core/UI/UIBridge.cs projects/Prisma2D/scripts/PrismaEngine.Core/UI/EventBus.cs
git commit -m "feat(ui): add UIBridge event handling with bubbling and EventBus"
```

---

## Task 7: Update PrismaEngine.Core.csproj

**Files:**
- Modify: `projects/Prisma2D/scripts/PrismaEngine.Core/PrismaEngine.Core.csproj`

- [ ] **Step 1: Update csproj to include UI folder**

```xml
<Project Sdk="Microsoft.NET.Sdk">
  <PropertyGroup>
    <TargetFramework>net10.0</TargetFramework>
    <ImplicitUsings>enable</ImplicitUsings>
    <Nullable>enable</Nullable>
    <AllowUnsafeBlocks>true</AllowUnsafeBlocks>
    <PublishTrimmed>true</PublishTrimmed>
  </PropertyGroup>

  <!-- Add this ItemGroup for UI folder -->
  <ItemGroup>
    <Compile Include="UI/**/*.cs" />
  </ItemGroup>
</Project>
```

- [ ] **Step 2: Commit**

```bash
git add projects/Prisma2D/scripts/PrismaEngine.Core/PrismaEngine.Core.csproj
git commit -m "chore(ui): add UI folder to PrismaEngine.Core project"
```

---

## Task 8: Build Verification

**Files:**
- Test: Build the C# project

- [ ] **Step 1: Build C# project**

```bash
cd projects/Prisma2D/scripts
dotnet build PrismaEngine.Core/PrismaEngine.Core.csproj
```

Expected: Build succeeds with no errors

- [ ] **Step 2: Build GameScripts to verify integration**

```bash
dotnet build GameScripts/GameScripts.csproj
```

Expected: Build succeeds with no errors

- [ ] **Step 3: Commit if not already committed**

```bash
git status
git commit -m "chore(ui): verify UI components build successfully"
```

---

## Summary

Phase 1 implements:
1. **UIElementType.cs** - Enum for UI element types
2. **AnchorPresets.cs** - Anchor flags and enums
3. **ColorBlock.cs** - Color state block struct
4. **Sprite.cs** - Sprite reference class
5. **UIComponent.cs** - Base UI component class with anchor system
6. **Image.cs** - Image component with ImageType support
7. **Button.cs** - Button component with state transitions
8. **Text.cs** - Text component with alignment
9. **UIBridge.cs** - Event handling with bubbling
10. **EventBus.cs** - Optional event bus system

**Estimated commits:** 7-8 commits
**Estimated time:** 2-4 hours

---

## Plan Complete

Two execution options:

**1. Subagent-Driven (recommended)** - I dispatch a fresh subagent per task, review between tasks, fast iteration

**2. Inline Execution** - Execute tasks in this session using executing-plans, batch execution with checkpoints

Which approach?
