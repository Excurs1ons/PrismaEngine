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
        if (index >= NativeAPI.API.GetEntityCapacity())
            throw new IndexOutOfRangeException("Handle OOB (entity does not exist)");
        if ((NativeAPI.RenderBuffer->Generation[index] & 0xFFFF) != gen) 
            throw new InvalidOperationException("Node Handle is STALE (Entity re-allocated)");
#endif
    }

    // Cherno Optimization: 属性访问直接指向对应的 SoA 缓冲区（�?冷分离）
    public float X { get { unsafe { Validate(); return NativeAPI.TransformBuffer_Read->PosX[Index]; } } set { unsafe { Validate(); NativeAPI.TransformBuffer_Write->PosX[Index] = value; } } }
    public float Y { get { unsafe { Validate(); return NativeAPI.TransformBuffer_Read->PosY[Index]; } } set { unsafe { Validate(); NativeAPI.TransformBuffer_Write->PosY[Index] = value; } } }
    public Vector2 Position { get { unsafe { Validate(); return new Vector2(NativeAPI.TransformBuffer_Read->PosX[Index], NativeAPI.TransformBuffer_Read->PosY[Index]); } } set { unsafe { Validate(); NativeAPI.TransformBuffer_Write->PosX[Index] = value.X; NativeAPI.TransformBuffer_Write->PosY[Index] = value.Y; } } }
    public float Rotation { get { unsafe { Validate(); return NativeAPI.TransformBuffer_Read->Rotation[Index]; } } set { unsafe { Validate(); NativeAPI.TransformBuffer_Write->Rotation[Index] = value; } } }
    public Vector2 Scale { get { unsafe { Validate(); return new Vector2(NativeAPI.TransformBuffer_Read->ScaleX[Index], NativeAPI.TransformBuffer_Read->ScaleY[Index]); } } set { unsafe { Validate(); NativeAPI.TransformBuffer_Write->ScaleX[Index] = value.X; NativeAPI.TransformBuffer_Write->ScaleY[Index] = value.Y; } } }

    public static Node Create(string name, World? world = null)
    {
        var targetWorld = world ?? World.Active ?? throw new InvalidOperationException("No active world.");
        uint handle;
        unsafe { handle = NativeAPI.API.CreateEntity(); }
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

    public bool Equals(Node other) => _handle == other._handle && _world == other._world;
    public override bool Equals(object? obj) => obj is Node other && Equals(other);
    public override int GetHashCode() => HashCode.Combine(_handle, _world);
}
