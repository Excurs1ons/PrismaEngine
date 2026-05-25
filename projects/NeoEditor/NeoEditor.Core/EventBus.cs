using System.Diagnostics;

namespace NeoEditor.Core;

/// <summary>
/// 事件总线: 单例模式，维护所有 WebUIBridge 实例。
/// 引擎状态 / 选择变更时广播到所有已注册的 WebUI 面板。
/// 线程安全 (锁定桥接列表)，支持 HTTP 轮询回退快照缓存。
/// </summary>
internal sealed class EventBus
{
    public static EventBus Instance { get; } = new();

    private readonly List<WebUIBridge> _bridges = new(capacity: 8);
    private readonly object _lock = new();

    // ---- 事件缓存 (供 HTTP 轮询回退使用) ----
    private long _generation;
    private string _cachedSelectionId = "";
    private string _cachedSceneTimestamp = "";
    private int _cachedFps;
    private string _cachedGpu = "N/A";
    private string _cachedScene = "None";
    private int _cachedObjects;

    private EventBus() { }

    // ================================================================
    // 桥接注册
    // ================================================================

    public void RegisterBridge(WebUIBridge bridge)
    {
        ArgumentNullException.ThrowIfNull(bridge);
        lock (_lock)
        {
            if (!_bridges.Contains(bridge))
                _bridges.Add(bridge);
        }
    }

    public void UnregisterBridge(WebUIBridge bridge)
    {
        lock (_lock)
        {
            _bridges.Remove(bridge);
        }
    }

    // ================================================================
    // 事件发布 (C# → WebUI 推送)
    // ================================================================

    /// <summary>
    /// 发布选择变更事件。entityId 为选中实体的 ID 字符串。
    /// </summary>
    public void PublishSelectionChanged(string entityId)
    {
        lock (_lock)
        {
            _cachedSelectionId = entityId;
            _generation++;
        }

        Broadcast(b => b.NotifySelectionChanged(entityId));
        Debug.WriteLine($"[EventBus] SelectionChanged: {entityId}");
    }

    /// <summary>
    /// 发布场景更新事件。
    /// </summary>
    public void PublishSceneUpdated()
    {
        string timestamp = DateTime.UtcNow.ToString("O");
        lock (_lock)
        {
            _cachedSceneTimestamp = timestamp;
            _generation++;
        }

        Broadcast(b => b.NotifySceneUpdated());
        Debug.WriteLine($"[EventBus] SceneUpdated: {timestamp}");
    }

    /// <summary>
    /// 发布引擎状态更新事件。
    /// </summary>
    public void PublishEngineStatus(int fps, string gpu, string scene, int objects)
    {
        lock (_lock)
        {
            _cachedFps = fps;
            _cachedGpu = gpu;
            _cachedScene = scene;
            _cachedObjects = objects;
            _generation++;
        }

        Broadcast(b => b.NotifyEngineStatus(fps, gpu, scene, objects));
    }

    // ================================================================
    // HTTP 轮询回退快照
    // ================================================================

    /// <summary>
    /// 获取当前事件快照 JSON，供 HTTP 轮询端点使用。
    /// WebUI 可比较 generation 值判断是否有新事件。
    /// </summary>
    public string GetEventsSnapshot()
    {
        lock (_lock)
        {
            return $$"""
            {
                "generation":{{_generation}},
                "selectionId":"{{EscapeJson(_cachedSelectionId)}}",
                "sceneTimestamp":"{{EscapeJson(_cachedSceneTimestamp)}}",
                "engineStatus":{
                    "fps":{{_cachedFps}},
                    "gpu":"{{EscapeJson(_cachedGpu)}}",
                    "scene":"{{EscapeJson(_cachedScene)}}",
                    "objects":{{_cachedObjects}}
                }
            }
            """;
        }
    }

    // ================================================================
    // 内部广播
    // ================================================================

    /// <summary>
    /// 对快照中的所有桥接实例执行操作。
    /// 快照机制避免死锁，允许在迭代过程中注册/注销。
    /// </summary>
    private void Broadcast(Action<WebUIBridge> action)
    {
        WebUIBridge[] snapshot;
        lock (_lock)
        {
            snapshot = _bridges.ToArray();
        }

        foreach (var bridge in snapshot)
        {
            try
            {
                action(bridge);
            }
            catch (Exception ex)
            {
                Debug.WriteLine($"[EventBus] Bridge error: {ex.Message}");
            }
        }
    }

    // ================================================================
    // Helpers
    // ================================================================

    private static string EscapeJson(string raw)
    {
        return raw
            .Replace("\\", "\\\\")
            .Replace("\"", "\\\"")
            .Replace("\n", "\\n")
            .Replace("\r", "\\r")
            .Replace("\t", "\\t");
    }
}
