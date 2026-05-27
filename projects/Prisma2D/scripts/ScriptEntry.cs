using System;
using System.Runtime.InteropServices;
using Prisma;

namespace GameScripts;

/// C++ 引擎通过 CoreCLR 调用的入口点�?
internal static class ScriptEntry
{
    [UnmanagedCallersOnly]
    public static void Bootstrap(IntPtr apiPtr)
    {
        System.Console.WriteLine("[Bootstrap] Entered");
        System.Console.Out.Flush();

        try
        {
            ScriptEngine.Bootstrap(apiPtr);

            // 注册所有源码生成的脚本
            GameScripts.Generated.ScriptRegistry.RegisterAll();

            // 创建场景，SceneInit 会创建摄像机 + 精灵
            var init = Node.Create("__SceneInit__");
            init.AddScript<SceneInit>();

            System.Console.WriteLine("[GameScripts] Scene initialized");
        }
        catch (Exception ex)
        {
            System.Console.Error.WriteLine("[FATAL] ScriptEntry.Bootstrap crashed: " + ex.GetType().Name + ": " + ex.Message);
            System.Console.Error.Flush();
            throw;
        }
    }

    [UnmanagedCallersOnly]
    public static void OnFrame(float dt)
    {
        ScriptEngine.OnFrame(dt);
    }
}
