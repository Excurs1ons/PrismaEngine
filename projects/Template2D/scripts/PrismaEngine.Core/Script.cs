namespace PrismaEngine;

/// <summary>
/// C# 脚本基类。用户脚本继承此类并重写生命周期方法。
/// </summary>
public abstract class Script
{
    /// <summary>所属 Node（类似 Unity 的 gameObject）。</summary>
    public Node? node { get; internal set; }

    // 快捷属性
    public float X { get => node?.X ?? 0; set { if (node != null) node.X = value; } }
    public float Y { get => node?.Y ?? 0; set { if (node != null) node.Y = value; } }
    public Vector2 Position { get => node?.Position ?? Vector2.Zero; set { if (node != null) node.Position = value; } }
    public float Rotation { get => node?.Rotation ?? 0; set { if (node != null) node.Rotation = value; } }
    public Vector2 Scale { get => node?.Scale ?? Vector2.One; set { if (node != null) node.Scale = value; } }

    /// <summary>脚本是否已调用 OnStart。</summary>
    internal bool _started;

    // ── Lifecycle ────────────────────────────────────────────

    /// <summary>脚本创建时调用（构造时）。</summary>
    public virtual void OnCreate() { }

    /// <summary>首次 OnUpdate 前调用。</summary>
    public virtual void OnStart() { }

    /// <summary>每帧调用。</summary>
    public virtual void OnUpdate(float dt) { }

    /// <summary>脚本被移除或 Node 销毁时调用。</summary>
    public virtual void OnDestroy() { }
}
