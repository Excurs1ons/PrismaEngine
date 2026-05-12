using System;
using System.Collections.Generic;

namespace Prisma.UI;

#region Event Types

public abstract class UIEvent
{
    public DateTime Timestamp { get; set; } = DateTime.Now;
}

public class ClickEvent : UIEvent
{
    public Node Target { get; set; }
    public Prisma.Vector2 ScreenPosition { get; set; }
}

public class PointerEnterEvent : UIEvent
{
    public Node Target { get; set; }
}

public class PointerExitEvent : UIEvent
{
    public Node Target { get; set; }
}

public class PointerDownEvent : UIEvent
{
    public Node Target { get; set; }
    public Prisma.Vector2 ScreenPosition { get; set; }
}

public class DragEvent : UIEvent
{
    public Node Target { get; set; }
    public Prisma.Vector2 ScreenPosition { get; set; }
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