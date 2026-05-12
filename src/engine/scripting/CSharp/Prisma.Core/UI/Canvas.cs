using System;
using System.Collections.Generic;

namespace Prisma.UI;

public enum CanvasRenderMode
{
    ScreenSpaceOverlay,
    ScreenSpaceCamera,
    WorldSpace
}

[Serializable]
public partial class Canvas : UIComponent
{
    public CanvasRenderMode RenderMode { get; set; } = CanvasRenderMode.ScreenSpaceOverlay;
    public float PlaneDistance { get; set; } = 100f;
    public int SortOrder { get; set; } = 0;
    
    private readonly List<Node> _elements = new();
    
    public Canvas()
    {
        ElementType = UIElementType.Canvas;
    }
    
    public override void OnCreate()
    {
        base.OnCreate();
        UIBridge.Instance.RegisterUIRoot(node);
    }
    
    public override void OnDestroy()
    {
        UIBridge.Instance.UnregisterUIRoot(node);
        base.OnDestroy();
    }
    
    public void AddElement(Node element)
    {
        if (element.Handle == 0) return;
        _elements.Add(element);
    }
    
    public void RemoveElement(Node element)
    {
        _elements.Remove(element);
    }
    
    internal List<Node> GetElements() => _elements;
}