using System;
using System.Collections.Generic;

namespace Prisma.UI;

public enum EaseType
{
    Linear,
    InQuad,
    OutQuad,
    InOutQuad,
    InCubic,
    OutCubic,
    InOutCubic
}

internal class FloatTween
{
    public float Elapsed { get; set; }
    public float Duration { get; set; }
    public float From { get; set; }
    public float To { get; set; }
    public float CurrentValue { get; set; }
    public Action<float>? OnUpdate { get; set; }
    public Action? OnComplete { get; set; }
}

internal class Vector2Tween
{
    public float Elapsed { get; set; }
    public float Duration { get; set; }
    public Vector2 From { get; set; }
    public Vector2 To { get; set; }
    public Vector2 CurrentValue { get; set; }
    public Action<Vector2>? OnUpdate { get; set; }
    public Action? OnComplete { get; set; }
}

public static class Tween
{
    private static readonly List<FloatTween> _floatTweens = new();
    private static readonly List<Vector2Tween> _vector2Tweens = new();
    
    public static void To(float from, float to, float duration, Action<float> onUpdate, Action? onComplete = null)
    {
        _floatTweens.Add(new FloatTween
        {
            From = from,
            To = to,
            Duration = duration,
            Elapsed = 0,
            CurrentValue = from,
            OnUpdate = onUpdate,
            OnComplete = onComplete
        });
    }
    
    public static void To(Vector2 from, Vector2 to, float duration, Action<Vector2> onUpdate, Action? onComplete = null)
    {
        _vector2Tweens.Add(new Vector2Tween
        {
            From = from,
            To = to,
            Duration = duration,
            Elapsed = 0,
            CurrentValue = from,
            OnUpdate = onUpdate,
            OnComplete = onComplete
        });
    }
    
    internal static void Update(float deltaTime)
    {
        for (int i = _floatTweens.Count - 1; i >= 0; i--)
        {
            var t = _floatTweens[i];
            
            t.Elapsed += deltaTime;
            float progress = Math.Min(t.Elapsed / t.Duration, 1f);
            
            t.CurrentValue = t.From + (t.To - t.From) * Ease(progress, EaseType.OutQuad);
            t.OnUpdate?.Invoke(t.CurrentValue);
            
            if (progress >= 1f)
            {
                t.OnComplete?.Invoke();
                _floatTweens.RemoveAt(i);
            }
        }
        
        for (int i = _vector2Tweens.Count - 1; i >= 0; i--)
        {
            var t = _vector2Tweens[i];
            
            t.Elapsed += deltaTime;
            float progress = Math.Min(t.Elapsed / t.Duration, 1f);
            
            float easeT = Ease(progress, EaseType.OutQuad);
            t.CurrentValue = new Vector2(
                t.From.X + (t.To.X - t.From.X) * easeT,
                t.From.Y + (t.To.Y - t.From.Y) * easeT);
            
            t.OnUpdate?.Invoke(t.CurrentValue);
            
            if (progress >= 1f)
            {
                t.OnComplete?.Invoke();
                _vector2Tweens.RemoveAt(i);
            }
        }
    }
    
    public static float Ease(float t, EaseType type)
    {
        return type switch
        {
            EaseType.Linear => t,
            EaseType.InQuad => t * t,
            EaseType.OutQuad => t * (2f - t),
            EaseType.InOutQuad => t < 0.5f ? 2f * t * t : -1f + (4f - 2f * t) * t,
            EaseType.InCubic => t * t * t,
            EaseType.OutCubic => (--t) * t * t + 1f,
            EaseType.InOutCubic => t < 0.5f ? 4f * t * t * t : (t - 1f) * (2f * t - 2f) * (2f * t - 2f) + 1f,
            _ => t
        };
    }
    
    public static void StopAll()
    {
        _floatTweens.Clear();
        _vector2Tweens.Clear();
    }
}