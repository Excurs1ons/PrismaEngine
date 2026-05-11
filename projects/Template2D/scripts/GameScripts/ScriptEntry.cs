using System;
using System.Runtime.InteropServices;
using PrismaEngine;

namespace GameScripts;

/// <summary>
/// C++ 引擎通过 CoreCLR 调用的入口点。
/// </summary>
internal static class ScriptEntry
{
    [UnmanagedCallersOnly]
    public static void Bootstrap(IntPtr apiPtr)
    {
        ScriptEngine.Bootstrap(apiPtr);

        // 创建场景 → SceneInit 会创建摄像机 + 精灵
        var init = Node.Create("__SceneInit__");
        init.AddScript<SceneInit>();

        System.Console.WriteLine("[GameScripts] Scene initialized");
    }

    [UnmanagedCallersOnly]
    public static void OnFrame(float dt)
    {
        ScriptEngine.OnFrame(dt);
    }
}
