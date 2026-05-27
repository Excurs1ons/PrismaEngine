using System;
using System.Runtime.InteropServices;
using Prisma;

namespace GameScripts;

internal static class ScriptEntry
{
    [UnmanagedCallersOnly]
    public static void Bootstrap(IntPtr apiPtr)
    {
        try
        {
            ScriptEngine.Bootstrap(apiPtr);
            Console.WriteLine("[MetroidvaniaDemo] Scripts bootstrapped successfully");
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine("[FATAL] ScriptEntry.Bootstrap crashed: " +
                ex.GetType().Name + ": " + ex.Message);
            throw;
        }
    }

    [UnmanagedCallersOnly]
    public static void OnFrame(float dt)
    {
        ScriptEngine.OnFrame(dt);
    }
}
