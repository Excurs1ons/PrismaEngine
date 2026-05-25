using System;
using System.Runtime.InteropServices;
using System.Text;

namespace NeoEditor.Core.Interop;

/// EditorAPI 函数指针表（与 C++ Prisma::Scripting::EditorAPI 内存布局一致）。
/// 25 个函数指针 + 1 个 uint32 structSize = 26 字段。
/// 布局: Scene(4) → Transform(6) → Selection(3) → Viewport(3) → Asset(3) → Editor(3) → Log(2) → Memory(1) → Size(1)
[StructLayout(LayoutKind.Sequential)]
internal unsafe struct EditorAPI_Interop
{
    // --- Scene (4) ---
    public delegate* unmanaged<IntPtr> SceneGetHierarchy;
    public delegate* unmanaged<byte*, ulong> SceneCreateEntity;
    public delegate* unmanaged<ulong, byte> SceneDeleteEntity;
    public delegate* unmanaged<ulong, byte*, byte> SceneRenameEntity;

    // --- Transform (6) ---
    public delegate* unmanaged<ulong, float*, float*, float*, void> TransformGetPosition;
    public delegate* unmanaged<ulong, float, float, float, void> TransformSetPosition;
    public delegate* unmanaged<ulong, float*, float*, float*, float*, void> TransformGetRotation;
    public delegate* unmanaged<ulong, float, float, float, float, void> TransformSetRotation;
    public delegate* unmanaged<ulong, float*, float*, float*, void> TransformGetScale;
    public delegate* unmanaged<ulong, float, float, float, void> TransformSetScale;

    // --- Selection (3) ---
    public delegate* unmanaged<ulong> SelectionGetSelected;
    public delegate* unmanaged<ulong, void> SelectionSetSelected;
    public delegate* unmanaged<void> SelectionClear;

    // --- Viewport (3) ---
    public delegate* unmanaged<uint*, uint*, void> ViewportGetSize;
    public delegate* unmanaged<uint, uint, void> ViewportSetSize;
    public delegate* unmanaged<byte*, byte> ViewportCaptureScreenshot;

    // --- Asset (3) ---
    public delegate* unmanaged<byte*, IntPtr> AssetBrowseDirectory;
    public delegate* unmanaged<byte*, IntPtr> AssetGetInfo;
    public delegate* unmanaged<byte*, byte> AssetImport;

    // --- Editor (3) ---
    public delegate* unmanaged<IntPtr> EditorGetStatus;
    public delegate* unmanaged<IntPtr> EditorGetEngineInfo;
    public delegate* unmanaged<byte*, byte> EditorExecuteCommand;

    // --- Log (2) ---
    public delegate* unmanaged<int, int, IntPtr> LogGetLogs;
    public delegate* unmanaged<void> LogClear;

    // --- Memory (1) ---
    // 释放 C++ strdup 分配的字符串。所有返回 IntPtr (JSON) 的函数
    // 都用 strdup 分配，C# 侧必须调用此函数释放。
    public delegate* unmanaged<void*, void> FreeString;

    // --- Struct version ---
    // C++ FillEditorAPI 设为 sizeof(EditorAPI)，C# Init 校验是否匹配
    public uint StructSize;
}

/// EditorAPI 高级封装。类型安全包装，自动处理字符串编组和内存释放。
public static unsafe class EditorAPI
{
    private static EditorAPI_Interop s_api;
    private static bool s_initialized;

    internal static bool IsInitialized => s_initialized;

    internal static void Initialize(IntPtr apiPtr)
    {
        if (apiPtr == IntPtr.Zero)
            throw new ArgumentNullException(nameof(apiPtr));

        s_api = *(EditorAPI_Interop*)apiPtr;

        uint expectedSize = (uint)sizeof(EditorAPI_Interop);
        if (s_api.StructSize != 0 && s_api.StructSize != expectedSize)
        {
            var msg =
                $"EditorAPI 结构体版本不匹配！\n"
                + $"  C++ sizeof(EditorAPI) = {s_api.StructSize}\n"
                + $"  C#  sizeof(EditorAPI_Interop) = {expectedSize}\n"
                + $"  请确保 C++ EditorAPI.h 与 C# EditorAPI.cs 字段完全一致，\n"
                + $"  并重新编译 NeoEditor.Core。";
            throw new InvalidOperationException(msg);
        }

        s_initialized = true;
    }

    // ===================================================================
    // 字符串编组辅助
    // ===================================================================

    private static byte[] StringToUtf8(string s)
    {
        return Encoding.UTF8.GetBytes((s ?? "") + "\0");
    }

    private static string MarshalAndFree(IntPtr ptr)
    {
        if (ptr == IntPtr.Zero)
            return "";

        var result = Marshal.PtrToStringAnsi(ptr);
        s_api.FreeString((void*)ptr);
        return result ?? "";
    }

    // ===================================================================
    // Scene API
    // ===================================================================

    public static string GetHierarchy()
    {
        var ptr = s_api.SceneGetHierarchy();
        return MarshalAndFree(ptr);
    }

    public static ulong CreateEntity(string name)
    {
        fixed (byte* namePtr = StringToUtf8(name))
        {
            return s_api.SceneCreateEntity(namePtr);
        }
    }

    public static bool DeleteEntity(ulong id)
    {
        return s_api.SceneDeleteEntity(id) != 0;
    }

    public static bool RenameEntity(ulong id, string name)
    {
        fixed (byte* namePtr = StringToUtf8(name))
        {
            return s_api.SceneRenameEntity(id, namePtr) != 0;
        }
    }

    // ===================================================================
    // Transform API
    // ===================================================================

    public static void GetPosition(ulong id, out float x, out float y, out float z)
    {
        float lx = 0, ly = 0, lz = 0;
        s_api.TransformGetPosition(id, &lx, &ly, &lz);
        x = lx;
        y = ly;
        z = lz;
    }

    public static void SetPosition(ulong id, float x, float y, float z)
    {
        s_api.TransformSetPosition(id, x, y, z);
    }

    public static void GetRotation(
        ulong id,
        out float x,
        out float y,
        out float z,
        out float w
    )
    {
        float lx = 0, ly = 0, lz = 0, lw = 1;
        s_api.TransformGetRotation(id, &lx, &ly, &lz, &lw);
        x = lx;
        y = ly;
        z = lz;
        w = lw;
    }

    public static void SetRotation(ulong id, float x, float y, float z, float w)
    {
        s_api.TransformSetRotation(id, x, y, z, w);
    }

    public static void GetScale(ulong id, out float x, out float y, out float z)
    {
        float lx = 0, ly = 0, lz = 0;
        s_api.TransformGetScale(id, &lx, &ly, &lz);
        x = lx;
        y = ly;
        z = lz;
    }

    public static void SetScale(ulong id, float x, float y, float z)
    {
        s_api.TransformSetScale(id, x, y, z);
    }

    // ===================================================================
    // Selection API
    // ===================================================================

    public static ulong GetSelectedEntity()
    {
        return s_api.SelectionGetSelected();
    }

    public static void SetSelectedEntity(ulong id)
    {
        s_api.SelectionSetSelected(id);
    }

    public static void ClearSelection()
    {
        s_api.SelectionClear();
    }

    // ===================================================================
    // Viewport API
    // ===================================================================

    public static void GetViewportSize(out uint w, out uint h)
    {
        uint lw = 0, lh = 0;
        s_api.ViewportGetSize(&lw, &lh);
        w = lw;
        h = lh;
    }

    public static void SetViewportSize(uint w, uint h)
    {
        s_api.ViewportSetSize(w, h);
    }

    public static bool CaptureScreenshot(string path)
    {
        fixed (byte* pathPtr = StringToUtf8(path))
        {
            return s_api.ViewportCaptureScreenshot(pathPtr) != 0;
        }
    }

    // ===================================================================
    // Asset API
    // ===================================================================

    public static string BrowseDirectory(string path)
    {
        fixed (byte* pathPtr = StringToUtf8(path))
        {
            var ptr = s_api.AssetBrowseDirectory(pathPtr);
            return MarshalAndFree(ptr);
        }
    }

    public static string GetAssetInfo(string path)
    {
        fixed (byte* pathPtr = StringToUtf8(path))
        {
            var ptr = s_api.AssetGetInfo(pathPtr);
            return MarshalAndFree(ptr);
        }
    }

    public static bool ImportAsset(string sourcePath)
    {
        fixed (byte* pathPtr = StringToUtf8(sourcePath))
        {
            return s_api.AssetImport(pathPtr) != 0;
        }
    }

    // ===================================================================
    // Editor API
    // ===================================================================

    public static string GetStatus()
    {
        var ptr = s_api.EditorGetStatus();
        return MarshalAndFree(ptr);
    }

    public static string GetEngineInfo()
    {
        var ptr = s_api.EditorGetEngineInfo();
        return MarshalAndFree(ptr);
    }

    public static bool ExecuteCommand(string cmd)
    {
        fixed (byte* cmdPtr = StringToUtf8(cmd))
        {
            return s_api.EditorExecuteCommand(cmdPtr) != 0;
        }
    }

    // ===================================================================
    // Log API
    // ===================================================================

    public static string GetLogs(int level, int count)
    {
        var ptr = s_api.LogGetLogs(level, count);
        return MarshalAndFree(ptr);
    }

    public static void ClearLogs()
    {
        s_api.LogClear();
    }

    // ===================================================================
    // Internal — test support
    // ===================================================================

    internal static void Reset()
    {
        s_api = default;
        s_initialized = false;
    }
}
