# PrismaEngine C# UI System Design

**Date**: 2026-05-12
**Author**: JasonGu
**Status**: Approved

---

## Overview

Implement a complete UI system in C# referencing Unity uGUI and NGUI patterns. The system integrates with existing C# scripting (Node/Script/World) and supports extensible architecture for future UI Pass extraction.

---

## Design Goals

1. **Scene Integration**: UI elements use existing Node/Script/World system, not isolated
2. **NGUI-Style Anchors**: Full anchor/pivot/offset system like NGUI
3. **Multi-Event System**: Support event bubbling, Script callbacks, and EventBus
4. **Multi-Font Support**: Bitmap, TrueType, and SDF fonts
5. **Phased Delivery**: Incremental implementation in phases

---

## Architecture

### Core Class Hierarchy

```
Script (existing)
├── UIComponent (new)
│   ├── Image (new)
│   │   └── Button (new)
│   ├── Text (new)
│   ├── Panel (Phase 2)
│   └── ScrollView (Phase 2)
└── (other game scripts)
```

### UIElement Type Enum

```csharp
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

### Anchor Presets (NGUI Style)

```csharp
[Flags]
public enum AnchorPresets
{
    None        = 0,
    TopLeft     = 1 << 0,
    TopCenter   = 1 << 1,
    TopRight    = 1 << 2,
    MiddleLeft  = 1 << 3,
    MiddleCenter= 1 << 4,
    MiddleRight = 1 << 5,
    BottomLeft  = 1 << 6,
    BottomCenter= 1 << 7,
    BottomRight = 1 << 8,
    StretchAll  = 1 << 10,  // Stretch edges
    Custom      = 1 << 11    // Custom anchor values
}
```

---

## Phase 1: UI Foundation (P0)

### 1.1 UIComponent Base Class

**File**: `PrismaEngine.Core/UI/UIComponent.cs`

```csharp
public abstract class UIComponent : Script
{
    // Position & Size
    public Vector2 AnchoredPosition { get; set; }
    public Vector2 Size { get; set; }
    
    // Anchor System (NGUI Style)
    public AnchorPresets AnchorPreset { get; set; }
    public Vector2 AnchorMin { get; set; }
    public Vector2 AnchorMax { get; set; }
    public Vector2 Pivot { get; set; }
    public Vector2 OffsetMin { get; set; }
    public Vector2 OffsetMax { get; set; }
    
    // Computed Properties
    public Vector2 WorldPosition { get; }
    public Rect Rect { get; }
    
    // Render Properties
    public Color Color { get; set; }
    public int SortingOrder { get; set; }
    public bool IsRaycastTarget { get; set; }
    
    // Hierarchy
    public Node? Parent { get; set; }
    public List<Node> Children { get; }
    
    // Event Callbacks
    public virtual void OnClick() { }
    public virtual void OnPointerEnter() { }
    public virtual void OnPointerExit() { }
    public virtual void OnPointerDown() { }
    public virtual void OnPointerUp() { }
    public virtual void OnBeginDrag() { }
    public virtual void OnDrag() { }
    public virtual void OnEndDrag() { }
    
    // Event Bubbling Control
    public bool IsStopBubbling { get; set; }
}
```

### 1.2 Image Component

**File**: `PrismaEngine.Core/UI/Image.cs`

```csharp
[Serializable]
public partial class Image : UIComponent
{
    public Sprite? Sprite { get; set; }
    public ImageType Type { get; set; } = ImageType.Simple;
    public bool FillCenter { get; set; } = true;
    public float FillAmount { get; set; } = 1f;
    public bool PreserveAspect { get; set; }
}

public enum ImageType
{
    Simple,      // Single sprite
    Sliced,      // 9-slice sprite
    Tiled,       // Tile sprite
    Filled       // Partial fill
}
```

### 1.3 Button Component

**File**: `PrismaEngine.Core/UI/Button.cs`

```csharp
[Serializable]
public partial class Button : Image
{
    public ColorBlock Colors { get; set; } = ColorBlock.Default;
    public SpriteState SpriteState { get; set; }
    public Navigation Navigation { get; set; }
    public bool IsInteractable { get; set; } = true;
    
    public override void OnClick()
    {
        if (!IsInteractable) return;
        // Handle click
    }
}

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
        DisabledColor = new Color(0.5f, 0.5f, 0.5f, 0.5f),
        ColorMultiplier = 1f
    };
}
```

### 1.4 Text Component

**File**: `PrismaEngine.Core/UI/Text.cs`

```csharp
[Serializable]
public partial class Text : UIComponent
{
    public string TextContent { get; set; } = "";
    public FontStyle FontStyle { get; set; } = FontStyle.Normal;
    public int FontSize { get; set; } = 14;
    public Color TextColor { get; set; } = Color.White;
    public TextAlignment Alignment { get; set; } = TextAlignment.MiddleCenter;
    public TextAnchor VerticalAnchor { get; set; } = TextAnchor.Middle;
    public TextAnchor HorizontalAnchor { get; set; } = TextAnchor.Center;
    public float LineSpacing { get; set; } = 1f;
    public bool RichText { get; set; } = true;
    public bool AutoSize { get; set; } = false;
    public Vector2 Margins { get; set; }
}

public enum FontStyle
{
    Normal = 0,
    Bold = 1,
    Italic = 2,
    BoldItalic = 3
}

public enum TextAlignment
{
    TopLeft, TopCenter, TopRight,
    MiddleLeft, MiddleCenter, MiddleRight,
    BottomLeft, BottomCenter, BottomRight
}
```

---

## Data Layer Design

### RenderBuffer SoA Extension

Extend existing `RenderBufferSoA` with UI fields:

```csharp
struct UIRenderBufferSoA {
    // Existing fields (from RenderBufferSoA)
    uint*  active;
    uint*  generation;
    float* colorR, colorG, colorB, colorA;
    float* sizeW, sizeH;
    
    // New UI fields
    uint*  uiType;              // UIElementType enum
    uint*  uiSortingOrder;      // Sorting order
    uint*  uiAnchorPreset;      // AnchorPresets enum
    float* uiAnchorMinX, uiAnchorMinY;
    float* uiAnchorMaxX, uiAnchorMaxY;
    float* uiPivotX, uiPivotY;
    float* uiOffsetMinX, uiOffsetMinY;
    float* uiOffsetMaxX, uiOffsetMaxY;
    uint*  uiIsRaycastTarget;
    uint*  uiFillAmount;        // For Image.FillAmount
};
```

### PrismaAPI Extension

```csharp
struct PrismaAPI {
    // Existing 18 functions...
    
    // New UI functions
    bool (*isPointerOverUI)();
    uint (*createUIElement)(UIElementType type);
    void (*setUIActive)(uint handle, bool active);
    void (*setUISortingOrder)(uint handle, int order);
    void (*setUIAnchorPreset)(uint handle, uint preset);
    void (*setUIPivot)(uint handle, float x, float y);
    void (*setUIOffsets)(uint handle, float minX, float minY, float maxX, float maxY);
    void (*setUISprite)(uint handle, uint spriteId);
    void (*setUIText)(uint handle, uint textId);
};
```

---

## Event System

### Three Event Modes

1. **Event Bubbling** (A): Pointer events bubble up through parent hierarchy
2. **Script Callbacks** (B): Virtual methods on UIComponent subclasses
3. **EventBus** (C): Centralized event pub/sub system

### Event Bus (Optional)

```csharp
public static class EventBus
{
    public static void Subscribe<T>(Action<T> handler) where T : UIEvent;
    public static void Unsubscribe<T>(Action<T> handler) where T : UIEvent;
    public static void Publish<T>(T eventData) where T : UIEvent;
}

public abstract class UIEvent { }
public class ClickEvent : UIEvent { public Node Target { get; set; } }
public class PointerEnterEvent : UIEvent { public Node Target { get; set; } }
```

### UIBridge

```csharp
public static class UIBridge
{
    public static UIBridge Instance { get; internal set; }
    
    public void ProcessPointerClick(Vector2 screenPos)
    {
        var hit = RaycastUI(screenPos);
        if (hit == null) return;
        
        // Event bubbling (A)
        BubbleEvent(hit, "OnClick");
        
        // Direct callback (B) - handled in BubbleEvent
        // EventBus (C) - publish event
        EventBus.Publish(new ClickEvent { Target = hit });
    }
    
    private void BubbleEvent(Node target, string methodName)
    {
        var current = target;
        while (current != null)
        {
            var ui = current.GetScript<UIComponent>();
            if (ui?.IsRaycastTarget == true && ui.IsStopBubbling)
            {
                InvokeMethod(ui, methodName);
                break;
            }
            InvokeMethod(ui, methodName);
            current = current.Parent;
        }
    }
}
```

---

## Input Priority

### Priority System (ABC All Supported)

1. **UI Priority**: Check UI hit first before game objects
2. **Layer-based Priority**: Camera.renderingOrder / SortingLayer
3. **Raycast Grouping**: Physics raycast can ignore UI or use separate group

```csharp
public enum InputPriority
{
    UIFirst = 0,     // A: UI always takes priority
    LayerBased = 1, // B: Based on camera layer order
    GameFirst = 2   // C: Game objects take priority
}
```

---

## Animation System

### Tween System (Property Interpolation)

```csharp
public static class Tween
{
    public static TweenProperty To(Node target, float duration, Action<float> onUpdate);
    public static TweenPosition PositionTo(Node target, Vector2 pos, float duration);
    public static TweenScale ScaleTo(Node target, Vector2 scale, float duration);
    public static TweenColor ColorTo(Image target, Color color, float duration);
    public static TweenAlpha AlphaTo(UIComponent target, float alpha, float duration);
}

public class Tween
{
    public bool IsPlaying { get; }
    public float Progress { get; }
    public TweenCallback OnComplete { get; set; }
    
    public void Play();
    public void PlayForward();
    public void PlayBackwards();
    public void Pause();
    public void Stop();
    public void Kill();
}
```

---

## File Structure

```
PrismaEngine.Core/
├── UI/
│   ├── UIComponent.cs        # Base class
│   ├── Image.cs              # Image component
│   ├── Button.cs             # Button component
│   ├── Text.cs               # Text component
│   ├── UIBridge.cs           # Event handling bridge
│   ├── AnchorPresets.cs      # Anchor enum and helpers
│   ├── UITransform.cs        # Transform extensions
│   ├── ColorBlock.cs         # Color block struct
│   ├── Sprite.cs             # Sprite reference
│   ├── Tween.cs              # Animation system
│   ├── EventBus.cs           # Optional event bus
│   └── UIElementType.cs      # Element type enum
└── PrismaEngine.Core.csproj
```

---

## C++ Integration Points

### ScriptEngine.h Extension

```cpp
struct PrismaAPI {
    // Existing 18 functions...
    
    // New UI functions (to be implemented)
    bool (*isPointerOverUI)();
    uint (*createUIElement)(UIElementType type);
    void (*setUIActive)(uint handle, bool active);
    void (*setUISortingOrder)(uint handle, int order);
};
```

### Reserved UI Pass Interface

```cpp
// RenderSystemNew.h - Future UI Pass extraction
class IRenderPass {
public:
    virtual void Execute() = 0;
    virtual int GetPriority() = 0;
};

// UI Pass would be implemented in Phase 2+
// Currently uses existing render pipeline
```

---

## Phase Summary

| Phase | Contents | Status |
|-------|----------|--------|
| Phase 1 | UIComponent + Image + Button + Text | **Current** |
| Phase 2 | Canvas + Panel + ScrollView + InputCallbacks | Planned |
| Phase 3 | Flexbox Layout + Tween Animation + Event Bubbling | Planned |

---

## Implementation Notes

1. **Source Generator**: Use existing `PrismaScriptGenerator` for TypeId generation
2. **Serialization**: Implement `OnSerialize/OnDeserialize` for all UI components
3. **Memory**: UI elements share existing SoA buffers with game objects
4. **Thread Safety**: Follow existing double-buffer pattern for UI state

---

## Open Questions

1. [x] UI element types: ABCD (Image, Button, Text, Panel, ScrollView, Dropdown, etc.)
2. [x] Layout system: NGUI anchors + Flexbox layout
3. [x] Rendering backend: Reuse RenderBuffer + reserve UI Pass
4. [x] Hierarchy: Transform parent/child (reuse Node system)
5. [x] Event system: A+B+C (bubbling + callbacks + EventBus)
6. [x] Font system: ABC (Bitmap + TrueType + SDF)
7. [x] Input priority: ABC (UI first + Layer-based + Raycast groups)
8. [x] Animation: Tween (property interpolation)
9. [x] Localization: User-implemented (not built-in)
