using System.Diagnostics;
using System.Text.Json;
using Microsoft.UI.Xaml.Controls;
using Microsoft.Web.WebView2.Core;

namespace NeoEditor.Core;

/// WebMessage 双向通信桥接层。
/// 管理 WebView2 与 C# 之间的 WebMessage 通信。
///
/// 消息协议:
///   C# → WebUI (推送): {"type":"eventName","data":...}
///   WebUI → C# (接收): {"action":"actionName","data":...}
internal sealed class WebUIBridge : IDisposable
{
    private readonly WebView2 _webView;
    private bool _disposed;

    /// 当 WebUI 请求选择实体时触发。
    /// 参数: entityId (字符串)
    public event Action<string>? EntitySelected;

    /// 当 WebUI 请求创建实体时触发。
    /// 参数: JSON data 中的 name 字段
    public event Action<string>? EntityCreateRequested;

    /// 当 WebUI 请求删除实体时触发。
    /// 参数: entityId (ulong)
    public event Action<ulong>? EntityDeleteRequested;

    /// 当 WebUI 发送自定义命令时触发。
    /// 参数: 命令字符串
    public event Action<string>? CommandExecuted;

    public WebUIBridge(WebView2 webView)
    {
        ArgumentNullException.ThrowIfNull(webView);
        _webView = webView;
    }

    /// 注册 CoreWebView2.WebMessageReceived 事件并注册到 EventBus。
    /// 必须在调用 EnsureCoreWebView2Async() 之后调用。
    public void Initialize()
    {
        ObjectDisposedException.ThrowIf(_disposed, this);

        var core = _webView.CoreWebView2
            ?? throw new InvalidOperationException(
                "CoreWebView2 is not initialized. Call EnsureCoreWebView2Async before Initialize.");

        core.WebMessageReceived += OnWebMessageReceived;

        // 注册到事件总线，接收引擎状态/选择变更推送
        EventBus.Instance.RegisterBridge(this);

        Debug.WriteLine($"[WebUIBridge] Initialized for panel, registered with EventBus");
    }

    // ================================================================
    // C# → WebUI 推送
    // ================================================================

    /// 向 WebUI 发送原始 JSON 消息。
    public void PostWebMessage(string message)
    {
        if (_disposed || _webView.CoreWebView2 == null)
            return;

        _webView.CoreWebView2.PostWebMessageAsJson(message);
    }

    /// 向 WebUI 发送类型化事件。
    /// data 应为 JSON 字符串 (可以是原始 JSON 对象或数组)。
    public void PostEvent(string type, string data)
    {
        string json = $"{{\"type\":\"{SanitizeJsonString(type)}\",\"data\":{data}}}";
        PostWebMessage(json);
    }

    /// 发送选中实体变更事件。
    public void NotifySelectionChanged(string entityId)
    {
        PostEvent("selectionChanged", $"\"{SanitizeJsonString(entityId)}\"");
    }

    /// 发送场景更新事件。
    public void NotifySceneUpdated()
    {
        PostEvent("sceneUpdated", $"\"{DateTime.UtcNow:O}\"");
    }

    /// 发送引擎状态事件。
    public void NotifyEngineStatus(int fps, string gpu, string scene, int objects)
    {
        string data = $$"""
            {"fps":{{fps}},"gpu":"{{SanitizeJsonString(gpu)}}","scene":"{{SanitizeJsonString(scene)}}","objects":{{objects}}}
            """;
        PostEvent("engineStatus", data);
    }

    // ================================================================
    // WebUI → C# 接收
    // ================================================================

    private void OnWebMessageReceived(object? sender, CoreWebView2WebMessageReceivedEventArgs e)
    {
        try
        {
            string raw = e.TryGetWebMessageAsString();
            if (string.IsNullOrEmpty(raw))
                return;

            using var doc = JsonDocument.Parse(raw);
            JsonElement root = doc.RootElement;

            if (!root.TryGetProperty("action", out JsonElement actionEl))
                return;

            string action = actionEl.GetString() ?? string.Empty;
            JsonElement dataEl = root.TryGetProperty("data", out var d) ? d : default;

            RouteAction(action, dataEl);
        }
        catch (JsonException ex)
        {
            Debug.WriteLine($"[WebUIBridge] Invalid JSON from WebUI: {ex.Message}");
        }
    }

    private void RouteAction(string action, JsonElement data)
    {
        Debug.WriteLine($"[WebUIBridge] Routing action: {action}");

        switch (action)
        {
            case "createEntity":
                OnCreateEntity(data);
                break;
            case "deleteEntity":
                OnDeleteEntity(data);
                break;
            case "selectEntity":
                OnSelectEntity(data);
                break;
            case "executeCommand":
                OnExecuteCommand(data);
                break;
            default:
                Debug.WriteLine($"[WebUIBridge] Unknown action: {action}");
                break;
        }
    }

    private void OnCreateEntity(JsonElement data)
    {
        string name = "Entity";
        if (data.ValueKind == JsonValueKind.Object
            && data.TryGetProperty("name", out var nameProp))
        {
            name = nameProp.GetString() ?? "Entity";
        }

        Debug.WriteLine($"[WebUIBridge] Creating entity: {name}");
        EntityCreateRequested?.Invoke(name);
    }

    private void OnDeleteEntity(JsonElement data)
    {
        if (data.ValueKind != JsonValueKind.Object
            || !data.TryGetProperty("id", out var idProp))
            return;

        if (idProp.TryGetUInt64(out ulong id))
        {
            Debug.WriteLine($"[WebUIBridge] Deleting entity: {id}");
            EntityDeleteRequested?.Invoke(id);
        }
    }

    private void OnSelectEntity(JsonElement data)
    {
        if (data.ValueKind != JsonValueKind.Object
            || !data.TryGetProperty("id", out var idProp))
            return;

        string entityId = idProp.GetRawText();
        Debug.WriteLine($"[WebUIBridge] Selecting entity: {entityId}");
        EntitySelected?.Invoke(entityId);
    }

    private void OnExecuteCommand(JsonElement data)
    {
        string cmd = data.ValueKind == JsonValueKind.Object
            && data.TryGetProperty("cmd", out var cmdProp)
                ? cmdProp.GetString() ?? ""
                : "";

        Debug.WriteLine($"[WebUIBridge] Executing command: {cmd}");
        CommandExecuted?.Invoke(cmd);
    }

    // ================================================================
    // IDisposable
    // ================================================================

    public void Dispose()
    {
        if (_disposed)
            return;
        _disposed = true;

        // 从事件总线注销
        EventBus.Instance.UnregisterBridge(this);

        try
        {
            if (_webView.CoreWebView2 != null)
            {
                _webView.CoreWebView2.WebMessageReceived -= OnWebMessageReceived;
            }
        }
        catch (ObjectDisposedException)
        {
            // CoreWebView2 may already be disposed
        }

        Debug.WriteLine("[WebUIBridge] Disposed, unregistered from EventBus");
    }

    // ================================================================
    // Helpers
    // ================================================================

    private static string SanitizeJsonString(string raw)
    {
        return raw
            .Replace("\\", "\\\\")
            .Replace("\"", "\\\"")
            .Replace("\n", "\\n")
            .Replace("\r", "\\r")
            .Replace("\t", "\\t");
    }
}
