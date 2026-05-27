using System.Net;
using System.Text;
using System.Text.Json;
using NeoEditor.Core.Interop;

namespace NeoEditor.Core;

internal static class EditorCommands
{
    public static string Dispatch(HttpListenerRequest request)
    {
        string body;
        using (var reader = new StreamReader(request.InputStream, Encoding.UTF8))
            body = reader.ReadToEnd();

        string path = request.Url?.AbsolutePath ?? "";
        return (request.HttpMethod, path) switch
        {
            ("POST", "/api/v1/hierarchy/get") => HandleHierarchyGet(),
            ("POST", "/api/v1/entity/create") => HandleEntityCreate(body),
            ("POST", "/api/v1/entity/delete") => HandleEntityDelete(body),
            ("POST", "/api/v1/entity/get")    => HandleEntityGet(body),
            ("POST", "/api/v1/entity/update") => HandleEntityUpdate(body),
            ("POST", "/api/v1/engine/status") => HandleEngineStatus(),
            ("POST", "/api/v1/console/get")   => HandleConsoleGet(),
            ("POST", "/api/v1/assets/list")   => HandleAssetsList(),
            ("GET",  "/api/v1/viewport/scene") => """{"status":"viewport_stream"}""",
            ("POST", "/api/v1/events/subscribe") => HandleEventsSubscribe(body),
            _ => $"{{\"error\":\"Not found: {request.HttpMethod} {path}\"}}"
        };
    }

    // ---- Hierarchy ----

    private static string HandleHierarchyGet()
    {
        // TODO: 当 EditorAPI 暴露 SceneGetHierarchy (返回 JSON) 时替换为真实调用
        return """{"entities":[]}""";
    }

    // ---- Entity CRUD ----

    private static string HandleEntityCreate(string body)
    {
        try
        {
            string name = "Entity";
            if (!string.IsNullOrEmpty(body))
            {
                using var doc = JsonDocument.Parse(body);
                if (doc.RootElement.TryGetProperty("name", out var nameProp))
                    name = nameProp.GetString() ?? "Entity";
            }

            ulong id = EditorAPI.CreateEntity(name);
            return $"{{\"id\":{id}}}";
        }
        catch (Exception ex)
        {
            return $"{{\"error\":\"{EscapeJson(ex.Message)}\"}}";
        }
    }

    private static string HandleEntityDelete(string body)
    {
        try
        {
            using var doc = JsonDocument.Parse(body);
            ulong id = doc.RootElement.GetProperty("id").GetUInt64();
            bool success = EditorAPI.DeleteEntity(id);
            return $"{{\"success\":{Bool(success)}}}";
        }
        catch (Exception ex)
        {
            return $"{{\"error\":\"{EscapeJson(ex.Message)}\"}}";
        }
    }

    private static string HandleEntityGet(string body)
    {
        try
        {
            using var doc = JsonDocument.Parse(body);
            ulong id = doc.RootElement.GetProperty("id").GetUInt64();

            EditorAPI.GetPosition(id, out float px, out float py, out float pz);
            EditorAPI.GetRotation(id, out float rx, out float ry, out float rz, out float rw);
            EditorAPI.GetScale(id, out float sx, out float sy, out float sz);

            return $$"""
            {
                "name":"Entity_{{id}}",
                "components":[
                    {
                        "type":"Transform",
                        "data":{
                            "position":[{{F(px)}},{{F(py)}},{{F(pz)}}],
                            "rotation":[{{F(rx)}},{{F(ry)}},{{F(rz)}},{{F(rw)}}],
                            "scale":[{{F(sx)}},{{F(sy)}},{{F(sz)}}]
                        }
                    }
                ]
            }
            """;
        }
        catch (Exception ex)
        {
            return $"{{\"error\":\"{EscapeJson(ex.Message)}\"}}";
        }
    }

    private static string HandleEntityUpdate(string body)
    {
        try
        {
            using var doc = JsonDocument.Parse(body);
            var root = doc.RootElement;
            ulong id = root.GetProperty("id").GetUInt64();

            if (!root.TryGetProperty("data", out var data))
                return """{"success":false,"error":"missing data"}""";

            if (data.TryGetProperty("name", out var nameProp))
            {
                EditorAPI.RenameEntity(id, nameProp.GetString() ?? "");
            }

            if (data.TryGetProperty("transform", out var transform))
            {
                if (transform.TryGetProperty("position", out var pos) && pos.ValueKind == JsonValueKind.Array)
                {
                    var arr = ReadFloatArray(pos);
                    if (arr.Length >= 3)
                        EditorAPI.SetPosition(id, arr[0], arr[1], arr[2]);
                }
                if (transform.TryGetProperty("rotation", out var rot) && rot.ValueKind == JsonValueKind.Array)
                {
                    var arr = ReadFloatArray(rot);
                    if (arr.Length >= 4)
                        EditorAPI.SetRotation(id, arr[0], arr[1], arr[2], arr[3]);
                }
                if (transform.TryGetProperty("scale", out var scale) && scale.ValueKind == JsonValueKind.Array)
                {
                    var arr = ReadFloatArray(scale);
                    if (arr.Length >= 3)
                        EditorAPI.SetScale(id, arr[0], arr[1], arr[2]);
                }
            }

            return """{"success":true}""";
        }
        catch (Exception ex)
        {
            return $"{{\"success\":false,\"error\":\"{EscapeJson(ex.Message)}\"}}";
        }
    }

    // ---- Engine / Console / Assets (stubs) ----

    private static string HandleEngineStatus()
    {
        // TODO: 当 EditorAPI 暴露 EditorGetStatus (返回 JSON) 时替换为真实调用
        return """{"fps":0,"gpu":"N/A","scene":"None","objects":0}""";
    }

    private static string HandleConsoleGet()
    {
        // TODO: 当 EditorAPI 暴露 LogGetLogs (返回 JSON) 时替换为真实调用
        return """{"logs":[]}""";
    }

    private static string HandleAssetsList()
    {
        // TODO: 当 EditorAPI 暴露 AssetBrowseDirectory (返回 JSON) 时替换为真实调用
        return """{"assets":[]}""";
    }

    // ---- Events (HTTP 轮询回退) ----

    private static string HandleEventsSubscribe(string body)
    {
        // HTTP 轮询回退: WebUI 在非 WebView2 环境中通过此端点获取事件快照。
        // WebView2 环境中优先使用 WebMessage 实时推送。
        return EventBus.Instance.GetEventsSnapshot();
    }

    // ---- Helpers ----

    private static string Bool(bool value) => value ? "true" : "false";

    private static string F(float value) => value.ToString("G", System.Globalization.CultureInfo.InvariantCulture);

    private static string EscapeJson(string raw)
    {
        return raw
            .Replace("\\", "\\\\")
            .Replace("\"", "\\\"")
            .Replace("\n", "\\n")
            .Replace("\r", "\\r")
            .Replace("\t", "\\t");
    }

    private static float[] ReadFloatArray(JsonElement element)
    {
        var list = new List<float>();
        foreach (var item in element.EnumerateArray())
        {
            if (item.ValueKind == JsonValueKind.Number)
                list.Add((float)item.GetDouble());
        }
        return list.ToArray();
    }
}
