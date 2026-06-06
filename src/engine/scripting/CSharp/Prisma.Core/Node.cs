using System;
using System.Runtime.CompilerServices;

namespace Prisma;

public static class ScriptType<T> where T : Script
{
    public static uint Id => ScriptRegistry.GetId<T>();
}

internal static class ScriptTypeInfo
{
    // Runtime counter removed as we now use Source Generator hash-based IDs
}

public readonly struct Node : IEquatable<Node>
{
    private readonly uint _handle;
    private readonly World _world;

    public Node(uint handle, World world) { _handle = handle; _world = world; }
    public uint Handle => _handle;
    public World World => _world;

    private uint Index => _handle & 0xFFFF;

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private unsafe void Validate()
    {
#if DEBUG
        uint index = Index;
        uint gen = _handle >> 16;
        // 动态容量检查：超过当前已分配的实体槽位则无�?
        if (index >= Interop.API.GetEntityCapacity())
            throw new IndexOutOfRangeException("Handle OOB (entity does not exist)");
        if ((Interop.RenderData->Generation[index] & 0xFFFF) != gen) 
            throw new InvalidOperationException("Node Handle is STALE (Entity re-allocated)");
#endif
    }

    // 属性访问直接指向底层的数据布局缓冲区，实现零开销跨语言访问
    public float X { get { unsafe { Validate(); return Interop.TransformRead->PosX[Index]; } } set { unsafe { Validate(); Interop.TransformRead->PosX[Index] = value; } } }
    public float Y { get { unsafe { Validate(); return Interop.TransformRead->PosY[Index]; } } set { unsafe { Validate(); Interop.TransformRead->PosY[Index] = value; } } }
    public Vector2 Position { get { unsafe { Validate(); return new Vector2(Interop.TransformRead->PosX[Index], Interop.TransformRead->PosY[Index]); } } set { unsafe { Validate(); Interop.TransformRead->PosX[Index] = value.X; Interop.TransformRead->PosY[Index] = value.Y; } } }
    public float Rotation { get { unsafe { Validate(); return Interop.TransformRead->Rotation[Index]; } } set { unsafe { Validate(); Interop.TransformRead->Rotation[Index] = value; } } }
    public Vector2 Scale { get { unsafe { Validate(); return new Vector2(Interop.TransformRead->ScaleX[Index], Interop.TransformRead->ScaleY[Index]); } } set { unsafe { Validate(); Interop.TransformRead->ScaleX[Index] = value.X; Interop.TransformRead->ScaleY[Index] = value.Y; } } }

    public static Node Create(string name, World? world = null)
    {
        var targetWorld = world ?? World.Active ?? throw new InvalidOperationException("No active world.");
        uint handle;
        unsafe { handle = Interop.API.CreateEntity(); }
        var node = new Node(handle, targetWorld);
        targetWorld.RegisterNode(node);
        return node;
    }

    public void Destroy() => _world.QueueDestruction(_handle);

    public T AddScript<T>() where T : Script, new() 
    { 
        var s = ScriptConstructor<T>.Create();
        s.node = this; 
        _world.RegisterActiveScript(s); 
        s.OnCreate(); 
        return s; 
    }

    private static class ScriptConstructor<T> where T : Script, new()
    {
        private static readonly Func<T> _constructor = () => new T();
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static T Create() => _constructor();
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public T? GetScript<T>() where T : Script => _world.GetScriptFromNodeByTypeId<T>(_handle, ScriptType<T>.Id);

    // ===== 组件池访问 =====

    /// <summary>获取 entity 上类型 T 的组件引用。调用前应当用 HasComponent 确认存在性。</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public ref T GetComponent<T>() where T : unmanaged
    {
        return ref _world.GetOrCreatePool<T>().Get(_handle);
    }

    /// <summary>检查 entity 是否持有类型 T 的组件。</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public bool HasComponent<T>() where T : unmanaged
    {
        return _world.GetOrCreatePool<T>().Has(_handle);
    }

    /// <summary>添加组件到 entity。如已有则覆盖标记，返回引用。</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public ref T AddComponent<T>() where T : unmanaged
    {
        var pool = _world.GetOrCreatePool<T>();
        pool.Set(_handle);
        return ref pool.Get(_handle);
    }

    /// <summary>从 entity 移除组件并清零数据。</summary>
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public void RemoveComponent<T>() where T : unmanaged
    {
        _world.GetOrCreatePool<T>().Remove(_handle);
    }

    public bool Equals(Node other) => _handle == other._handle && _world == other._world;
    public override bool Equals(object? obj) => obj is Node other && Equals(other);
    public override int GetHashCode() => HashCode.Combine(_handle, _world);
}
