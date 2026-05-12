using System;
using System.Collections.Generic;
using System.Collections.Concurrent;
using System.Threading.Tasks;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Prisma;

public class World : IDisposable
{
    private const int ParallelThreshold = 64;

    private readonly List<Node> _nodes = new();
    private readonly Dictionary<Type, List<Script>> _updateBatches = new();
    private readonly Dictionary<Type, List<Script>> _lateUpdateBatches = new();
    private readonly List<List<Script>> _flattenedUpdateBatches = new();
    private readonly List<List<Script>> _flattenedLateUpdateBatches = new();

    private readonly ConcurrentQueue<Script> _pendingStartScripts = new();
    private readonly ConcurrentQueue<Script> _scriptsToRemove = new();
    private readonly ConcurrentQueue<uint> _destructionQueue = new();
    
    private Script?[] _nodeScripts = Array.Empty<Script?>();

    private void EnsureNodeScriptsIndex(uint index)
    {
        if (index >= _nodeScripts.Length)
        {
            int newSize = Math.Max((int)index + 1, Math.Max(_nodeScripts.Length * 2, 64));
            Array.Resize(ref _nodeScripts, newSize);
        }
    }

    public SpatialGrid SpatialGrid { get; } = new(128);
    public TimeContext Time { get; } = new();
    public InputContext Input { get; } = new();
    public static World? Active { get; internal set; }

    internal void RegisterNode(Node n) => _nodes.Add(n);

    internal void RegisterActiveScript(Script s)
    {
        uint index = s.node.Handle & 0xFFFF;
        EnsureNodeScriptsIndex(index);
        s._nextScript = _nodeScripts[index];
        _nodeScripts[index] = s;
        s._entityIndex = index;
        
        var batches = s.ExecutionPhase == UpdatePhase.Update ? _updateBatches : _lateUpdateBatches;
        var flatBatches = s.ExecutionPhase == UpdatePhase.Update ? _flattenedUpdateBatches : _flattenedLateUpdateBatches;
        
        var type = s.GetType();
        if (!batches.TryGetValue(type, out var list))
        {
            list = new List<Script>();
            batches[type] = list;
            flatBatches.Add(list);
        }
        list.Add(s);
        _pendingStartScripts.Enqueue(s);
    }

    internal T? GetScriptFromNodeByTypeId<T>(uint handle, uint typeId) where T : Script
    {
        uint index = handle & 0xFFFF;
        var s = _nodeScripts[index];
        while (s != null)
        {
            if (s.TypeId == typeId) return (T)s;
            s = s._nextScript;
        }
        return null;
    }

    internal void UnregisterActiveScript(Script s) => _scriptsToRemove.Enqueue(s);

    public void Step(float dt)
    {
        Input.UpdateState(); 
        UpdateTime(dt);
        Prisma.Time._deltaTime = this.Time.DeltaTime;
        Prisma.Time._elapsed = this.Time.Elapsed;
        Prisma.Time._timeScale = this.Time.TimeScale;

        // 每帧同步：将 Read 缓冲区的活跃数据拷贝�?Write 缓冲�?
        // 确保脚本只写增量时，Write 始终有完整的基�?
        int aliveCount;
        unsafe { aliveCount = (int)NativeAPI.API.GetEntityCapacity(); }
        SyncActiveBuffers(aliveCount);

        ProcessDestructionQueue();
        ProcessRemovalQueue();
        ProcessPendingStarts();

        // 1. 更新空间索引 (使用当前 Write 缓冲区的数据，为下一帧做准备)
        UpdateSpatialGrid();

        // 2. 并行执行逻辑
        Prisma.Time._isParallelStep = true;
        try
        {
            for (int i = 0; i < _flattenedUpdateBatches.Count; i++) DispatchBatch(_flattenedUpdateBatches[i]);
            for (int i = 0; i < _flattenedLateUpdateBatches.Count; i++) DispatchBatch(_flattenedLateUpdateBatches[i]);
        }
        finally { Prisma.Time._isParallelStep = false; }

        // 3. 交换读写缓冲�?(Double-Buffer Swap)
        // 本帧写的数据变成下一帧读的数据�?
        NativeAPI.SwapBuffers();
    }

    /// <summary>
    /// �?Read 缓冲区的活跃数据拷贝�?Write 缓冲区�?
    /// 消除"幽灵数据"闪烁：静止实体未写增量时，Write 中仍有正确的基值�?
    /// </summary>
    private unsafe void SyncActiveBuffers(int aliveCount)
    {
        long bytes = (long)aliveCount * sizeof(float);
        if (bytes <= 0) return;

        var r = NativeAPI.TransformBuffer_Read;
        var w = NativeAPI.TransformBuffer_Write;
        Buffer.MemoryCopy(r->PosX, w->PosX, bytes, bytes);
        Buffer.MemoryCopy(r->PosY, w->PosY, bytes, bytes);
        Buffer.MemoryCopy(r->Rotation, w->Rotation, bytes, bytes);
        Buffer.MemoryCopy(r->ScaleX, w->ScaleX, bytes, bytes);
        Buffer.MemoryCopy(r->ScaleY, w->ScaleY, bytes, bytes);
    }

    private unsafe void UpdateSpatialGrid()
    {
        SpatialGrid.Clear();
        int nodeCount = _nodes.Count;
        if (nodeCount == 0) return;

        if (nodeCount < ParallelThreshold)
        {
            // 小规模：串行构建，避�?Dictionary 竞�?
            for (int i = 0; i < nodeCount; i++)
            {
                uint handle = _nodes[i].Handle;
                uint idx = handle & 0xFFFF;
                SpatialGrid.UpdateEntity(handle,
                    new Vector2(NativeAPI.TransformBuffer_Write->PosX[idx],
                                NativeAPI.TransformBuffer_Write->PosY[idx]));
            }
        }
        else
        {
            // Cherno Optimization: 大规模实体时并行构建�?
            // 注意：SpatialGrid.UpdateEntity 修改 Dictionary，当前不是完全线程安全的�?
            // 在百万级实体方案中，应该使用并行计数排序（Parallel Radix Sort）�?
            Parallel.For(0, nodeCount, i =>
            {
                uint handle = _nodes[i].Handle;
                uint idx = handle & 0xFFFF;
                SpatialGrid.UpdateEntity(handle,
                    new Vector2(NativeAPI.TransformBuffer_Write->PosX[idx],
                                NativeAPI.TransformBuffer_Write->PosY[idx]));
            });
        }
    }

    private void ProcessPendingStarts()
    {
        while (_pendingStartScripts.TryDequeue(out var s))
        {
            if (!s._started) { s.OnStart(); s._started = true; }
        }
    }

    private void ProcessRemovalQueue()
    {
        while (_scriptsToRemove.TryDequeue(out var s))
        {
            var batches = s.ExecutionPhase == UpdatePhase.Update ? _updateBatches : _lateUpdateBatches;
            if (batches.TryGetValue(s.GetType(), out var list)) list.Remove(s);
            
            uint index = s.node.Handle & 0xFFFF;
            var current = _nodeScripts[index];
            Script? prev = null;
            while (current != null)
            {
                if (current == s)
                {
                    if (prev == null) _nodeScripts[index] = s._nextScript;
                    else prev._nextScript = s._nextScript;
                    break;
                }
                prev = current;
                current = current._nextScript;
            }
        }
    }

    private void ProcessDestructionQueue()
    {
        while (_destructionQueue.TryDequeue(out uint handle))
        {
            uint index = handle & 0xFFFF;
            var s = _nodeScripts[index];
            while (s != null)
            {
                s.OnDestroy();
                var batches = s.ExecutionPhase == UpdatePhase.Update ? _updateBatches : _lateUpdateBatches;
                if (batches.TryGetValue(s.GetType(), out var list)) list.Remove(s);
                s = s._nextScript;
            }
            _nodeScripts[index] = null;
            unsafe { NativeAPI.API.DestroyEntity(handle); }
        }
    }

    private void UpdateTime(float dt) { Time.UnscaledDeltaTime = dt; Time.DeltaTime = dt * Time.TimeScale; Time.Elapsed += Time.DeltaTime; }

    private void DispatchBatch(List<Script> list)
    {
        int count = list.Count;
        if (count == 0) return;

        if (count < ParallelThreshold)
        {
            for (int i = 0; i < count; i++) list[i].OnUpdate(Time, Input);
        }
        else
        {
            // Cherno Fix: 简化闭包捕获以避免编译错误和内存分配�?
            var time = this.Time;
            var input = this.Input;
            Parallel.For(0, count, i => 
            {
                try { list[i].OnUpdate(time, input); }
                catch (Exception e) { Debug.LogError($"Script Error: {e.Message}"); }
            });
        }
    }

    internal void QueueDestruction(uint handle) { _destructionQueue.Enqueue(handle); }

    public string SerializeScene()
    {
        using var stream = new System.IO.MemoryStream();
        using var writer = new System.Text.Json.Utf8JsonWriter(stream, new System.Text.Json.JsonWriterOptions { Indented = true });
        
        writer.WriteStartObject();
        writer.WriteString("SceneName", "PrismaScene");
        
        writer.WriteStartArray("Entities");
        foreach (var node in _nodes)
        {
            writer.WriteStartObject();
            writer.WriteNumber("Id", node.Handle);
            
            // Serialize Transform (Direct from SoA)
            writer.WriteStartObject("Transform");
            writer.WriteStartArray("Position");
            writer.WriteNumberValue(node.X);
            writer.WriteNumberValue(node.Y);
            writer.WriteEndArray();
            writer.WriteNumber("Rotation", node.Rotation);
            writer.WriteEndObject();
            
            // Serialize Scripts
            writer.WriteStartArray("Scripts");
            var s = _nodeScripts[node.Handle & 0xFFFF];
            while (s != null)
            {
                writer.WriteStartObject();
                writer.WriteNumber("TypeId", s.TypeId);
                writer.WriteStartObject("Data");
                s.OnSerialize(writer); // 调用 Source Generator 生成的代�?/ Call code generated by Source Generator
                writer.WriteEndObject();
                writer.WriteEndObject();
                s = s._nextScript;
            }
            writer.WriteEndArray();
            
            writer.WriteEndObject();
        }
        writer.WriteEndArray();
        writer.WriteEndObject();
        
        writer.Flush();
        return System.Text.Encoding.UTF8.GetString(stream.ToArray());
    }

    public void Dispose() 
    { 
        foreach (var node in _nodes) QueueDestruction(node.Handle);
        ProcessDestructionQueue();
        _nodes.Clear(); 
        _updateBatches.Clear(); 
        _lateUpdateBatches.Clear(); 
        _flattenedUpdateBatches.Clear();
        _flattenedLateUpdateBatches.Clear();
        for (int i = 0; i < _nodeScripts.Length; i++) _nodeScripts[i] = null;
    }
}
