using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Prisma;

/// <summary>
/// 内部脚本引擎。充�?C++ 引擎�?C# World 之间的桥梁�?
/// Internal script engine. Acts as a bridge between C++ engine and C# World.
/// </summary>
public static class ScriptEngine
{
    private static World? _mainWorld;

    /// <summary>SRP 渲染回调。由游戏脚本在 Bootstrap 中设置。</summary>
    public static Action<float>? OnRenderCallback;

    /// <summary>
    /// 引导入口。支持热重载时的状态迁移�?
    /// Bootstrap entry point. Supports state migration during Hot Reloading.
    /// </summary>
    public static void Bootstrap(IntPtr apiPtr)
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
                string projectName;
                unsafe { projectName = Interop.Utf8ToString(Interop.API.ProjectName); }
                if (string.IsNullOrEmpty(projectName)) projectName = "GameScripts";

                string assemblyName = projectName + ".Scripts";

                // 优先尝试 [Project].Scripts.dll，回退到 Prisma.Bindings.dll (引擎内置)
                var registryType = Type.GetType($"Prisma.Generated.ScriptRegistry, {assemblyName}") ??
                                   Type.GetType("Prisma.Generated.ScriptRegistry, Prisma.Bindings");

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
    public static void OnFrame(float dt)
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

    /// <summary>
    /// 每帧渲染回调。在 C++ BeginFrame/EndFrame 之间调用。
    /// 由游戏脚本的 OnRender 委托触发 SRP 管线。
    /// </summary>
    public static void OnRender(float dt)
    {
        try
        {
            OnRenderCallback?.Invoke(dt);
        }
        catch (Exception e)
        {
            Debug.LogError($"CRITICAL: SRP Render Failure: {e.Message}\n{e.StackTrace}");
        }
    }
}
