using System.Collections.Generic;

namespace PrismaEngine;

/// <summary>
/// 游戏实体。位置、旋转、缩放直接挂在 Node 上（无额外的 Transform 类）。
/// </summary>
public class Node
{
    internal uint _handle;
    internal List<Script> _scripts = new();
    internal bool _destroyed;

    public string Name { get; set; } = "";

    // ── Position ──────────────────────────────────────────────
    public float X
    {
        get
        {
            float x = 0, y = 0;
            unsafe { NativeAPI.API.GetPosition(_handle, &x, &y); }
            return x;
        }
        set
        {
            unsafe { NativeAPI.API.SetPosition(_handle, value, Y); }
        }
    }

    public float Y
    {
        get
        {
            float x = 0, y = 0;
            unsafe { NativeAPI.API.GetPosition(_handle, &x, &y); }
            return y;
        }
        set
        {
            unsafe { NativeAPI.API.SetPosition(_handle, X, value); }
        }
    }

    public Vector2 Position
    {
        get
        {
            float x = 0, y = 0;
            unsafe { NativeAPI.API.GetPosition(_handle, &x, &y); }
            return new(x, y);
        }
        set
        {
            unsafe { NativeAPI.API.SetPosition(_handle, value.X, value.Y); }
        }
    }

    // ── Rotation ─────────────────────────────────────────────
    public float Rotation
    {
        get
        {
            unsafe { return NativeAPI.API.GetRotation(_handle); }
        }
        set
        {
            unsafe { NativeAPI.API.SetRotation(_handle, value); }
        }
    }

    // ── Scale ─────────────────────────────────────────────────
    public float ScaleX
    {
        get
        {
            float x = 0, y = 0;
            unsafe { NativeAPI.API.GetScale(_handle, &x, &y); }
            return x;
        }
        set
        {
            unsafe { NativeAPI.API.SetScale(_handle, value, ScaleY); }
        }
    }

    public float ScaleY
    {
        get
        {
            float x = 0, y = 0;
            unsafe { NativeAPI.API.GetScale(_handle, &x, &y); }
            return y;
        }
        set
        {
            unsafe { NativeAPI.API.SetScale(_handle, ScaleX, value); }
        }
    }

    public Vector2 Scale
    {
        get
        {
            float x = 0, y = 0;
            unsafe { NativeAPI.API.GetScale(_handle, &x, &y); }
            return new(x, y);
        }
        set
        {
            unsafe { NativeAPI.API.SetScale(_handle, value.X, value.Y); }
        }
    }

    // ── Constructor ──────────────────────────────────────────

    public Node(string name)
    {
        unsafe { _handle = NativeAPI.API.CreateEntity(); }
        Name = name;
        ScriptEngine.RegisterNode(this);
    }

    ~Node()
    {
        if (!_destroyed)
        {
            _destroyed = true;
            unsafe { NativeAPI.API.DestroyEntity(_handle); }
        }
    }

    // ── Script management ────────────────────────────────────

    public T AddScript<T>() where T : Script, new()
    {
        var s = new T { node = this };
        _scripts.Add(s);
        s.OnCreate();
        return s;
    }

    public T? GetScript<T>() where T : Script
    {
        foreach (var s in _scripts)
            if (s is T t) return t;
        return null;
    }

    public void RemoveScript<T>() where T : Script
    {
        for (int i = _scripts.Count - 1; i >= 0; i--)
        {
            if (_scripts[i] is T)
            {
                _scripts[i].OnDestroy();
                _scripts.RemoveAt(i);
            }
        }
    }
}
