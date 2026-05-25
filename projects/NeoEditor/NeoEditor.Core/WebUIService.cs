using System.Net;
using System.Text;

namespace NeoEditor.Core;

/// 轻量级 HTTP 服务器，为 WebView2 面板提供 REST API 通信。
/// 使用 System.Net.HttpListener 实现，不需要 ASP.NET Core。
/// 集成 EventBus 事件广播，支持 HTTP 轮询回退。
///
/// Windows 权限说明:
///   HttpListener 需要注册 URL ACL。如果启动失败，以管理员身份运行一次:
///     netsh http add urlacl http://localhost:8080/ user=Everyone
///   或者使用管理员权限运行 NeoEditor。
internal sealed class WebUIService : IDisposable
{
    private readonly HttpListener _listener;
    private readonly CancellationTokenSource _cts = new();
    private Task? _listenTask;
    private bool _disposed;

    public int Port { get; }
    public bool IsRunning => _listener.IsListening;

    public WebUIService(int port = 8080)
    {
        Port = port;
        _listener = new HttpListener();
        _listener.Prefixes.Add($"http://localhost:{port}/");
    }

    public void Start()
    {
        if (_disposed) throw new ObjectDisposedException(nameof(WebUIService));
        if (_listener.IsListening) return;

        _listener.Start();
        _listenTask = Task.Run(() => ListenLoop(_cts.Token));
    }

    public void Stop()
    {
        _cts.Cancel();
        try { _listener.Stop(); }
        catch (ObjectDisposedException) { }
        catch (HttpListenerException) { }
    }

    private async Task ListenLoop(CancellationToken ct)
    {
        while (!ct.IsCancellationRequested)
        {
            try
            {
                var ctx = await _listener.GetContextAsync().WaitAsync(ct);
                _ = HandleRequestAsync(ctx);
            }
            catch (OperationCanceledException)
            {
                break;
            }
            catch (HttpListenerException) when (ct.IsCancellationRequested)
            {
                break;
            }
            catch (ObjectDisposedException)
            {
                break;
            }
        }
    }

    private async Task HandleRequestAsync(HttpListenerContext ctx)
    {
        try
        {
            if (ctx.Request.HttpMethod == "OPTIONS")
            {
                ctx.Response.StatusCode = 204;
                ctx.Response.Headers.Add("Access-Control-Allow-Origin", "*");
                ctx.Response.Headers.Add("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
                ctx.Response.Headers.Add("Access-Control-Allow-Headers", "Content-Type");
                ctx.Response.ContentLength64 = 0;
                ctx.Response.OutputStream.Close();
                return;
            }

            var response = EditorCommands.Dispatch(ctx.Request);
            byte[] buffer = Encoding.UTF8.GetBytes(response);

            ctx.Response.ContentType = "application/json";
            ctx.Response.ContentLength64 = buffer.Length;
            ctx.Response.Headers.Add("Access-Control-Allow-Origin", "*");
            ctx.Response.Headers.Add("Cache-Control", "no-cache, no-store, must-revalidate");

            await ctx.Response.OutputStream.WriteAsync(buffer);
        }
        catch (Exception ex)
        {
            byte[] error = Encoding.UTF8.GetBytes(
                $"{{\"error\":\"{SanitizeJsonString(ex.Message)}\"}}");
            ctx.Response.StatusCode = 500;
            ctx.Response.ContentType = "application/json";
            ctx.Response.Headers.Add("Access-Control-Allow-Origin", "*");
            await ctx.Response.OutputStream.WriteAsync(error);
        }
        finally
        {
            try { ctx.Response.OutputStream.Close(); }
            catch (ObjectDisposedException) { }
            catch (HttpListenerException) { }
        }
    }

    private static string SanitizeJsonString(string raw)
    {
        return raw
            .Replace("\\", "\\\\")
            .Replace("\"", "\\\"")
            .Replace("\n", "\\n")
            .Replace("\r", "\\r")
            .Replace("\t", "\\t");
    }

    public void Dispose()
    {
        if (_disposed) return;
        _disposed = true;
        Stop();
        _cts.Dispose();
        (_listener as IDisposable)?.Dispose();
    }
}
