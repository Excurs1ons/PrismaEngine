using System;
using System.Runtime.InteropServices;
using Prisma;

namespace GameScripts;

internal static class ScriptEntry
{
    [UnmanagedCallersOnly]
    public static void Bootstrap(IntPtr apiPtr)
    {
        ScriptEngine.Bootstrap(apiPtr);
        GameScripts.Generated.ScriptRegistry.RegisterAll();

        var init = Node.Create("__SceneInit__");
        init.AddScript<SceneInit>();

        System.Console.WriteLine("[GameScripts] Template3D Scene initialized");
    }

    [UnmanagedCallersOnly]
    public static void OnFrame(float dt)
    {
        ScriptEngine.OnFrame(dt);
    }
}
