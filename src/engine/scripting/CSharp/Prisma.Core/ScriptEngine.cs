using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Prisma;

/// <summary>
/// 内部脚本引擎。充�?C++ 引擎�?C# World 之间的桥梁�?
/// Internal script engine. Acts as a bridge between C++ engine and C# World.
/// </summary>
internal static class ScriptEngine
{
    private static World? _mainWorld;

    /// <summary>
    /// 引导入口。支持热重载时的状态迁移�?
    /// Bootstrap entry point. Supports state migration during Hot Reloading.
    /// </summary>
    internal static void Bootstrap(IntPtr apiPtr)
    {
        unsafe { Interop.Init((PrismaAPI*)apiPtr); }
        
        // Cherno: 如果是在热重载过程中，我们可能需要保留之前的 World 状�?
        if (_mainWorld == null)
        {
            _mainWorld = new World();
            World.Active = _mainWorld;

            // 调用自动生成的脚本注册逻辑
            // Call automatically generated script registration logic
            try { 
                // 使用反射尝试调用，以�?Generator 还没运行
                var registryType = Type.GetType("Prisma.Generated.ScriptRegistry, GameScripts") ?? 
                                   Type.GetType("Prisma.Generated.ScriptRegistry, Prisma.Core");
                registryType?.GetMethod("RegisterAll")?.Invoke(null, null);
            } catch { /* Ignore */ }
        }
        else
        {
            Debug.Log("Hot Reload detected: Migrating state...");
            // TODO: 在这里实现跨程序集的脚本实例迁移逻辑
        }
    }

    /// <summary>
    /// 每帧更新逻辑�?
    /// Update logic per frame.
    /// </summary>
    internal static void OnFrame(float dt)
    {
        try
        {
            _mainWorld?.Step(dt);
        }
        catch (Exception e)
        {
            Debug.LogError($"CRITICAL: ScriptEngine Execution Failure: {e.Message}\n{e.StackTrace}");
        }
    }
}
