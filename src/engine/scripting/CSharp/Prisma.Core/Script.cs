using System;

namespace Prisma;

public enum UpdatePhase { None = 0, Update = 1, LateUpdate = 2 }

public abstract class Script
{
    internal Node _node;
    public Node node { get => _node; internal set => _node = value; }

    internal Script? _nextScript;
    internal uint _entityIndex;

    public abstract uint TypeId { get; }

    // Transform 属性委托给 Node（使用 _node 字段避免 CS1612 值类型复制问题）
    public Vector2 Position { get => _node.Position; set => _node.Position = value; }
    public float Rotation { get => _node.Rotation; set => _node.Rotation = value; }
    public Vector2 Scale { get => _node.Scale; set => _node.Scale = value; }

    internal bool _started;
    public virtual UpdatePhase ExecutionPhase => UpdatePhase.Update;

    public virtual void OnCreate() { }
    public virtual void OnStart() { }
    public virtual void OnUpdate(TimeContext time, InputContext input) { }
    public virtual void OnLateUpdate(TimeContext time, InputContext input) { }
    public virtual void OnDrawGizmos() { }
    public virtual void OnDestroy() { }

    // ===== 组件访问快捷方式（委托给 node） =====
    protected ref T GetComponent<T>() where T : unmanaged => ref node.GetComponent<T>();
    protected bool HasComponent<T>() where T : unmanaged => node.HasComponent<T>();
    protected ref T AddComponent<T>() where T : unmanaged => ref node.AddComponent<T>();
    protected void RemoveComponent<T>() where T : unmanaged => node.RemoveComponent<T>();

    // 序列化支�?/ Serialization Support
    public virtual void OnSerialize(System.Text.Json.Utf8JsonWriter writer) { }
    public virtual void OnDeserialize(ref System.Text.Json.Utf8JsonReader reader) { }
}
