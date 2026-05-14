using System;
using System.Runtime.InteropServices;
using Prisma;
using Prisma.SRP;

namespace GameScripts;

/// <summary>
/// C++ CoreCLR 入口点。Bootstrap → OnFrame → OnRender 生命周期。
/// OnRender 驱动 SRP 管线。
/// </summary>
internal static class ScriptEntry
{
    private static ShaderpackPipeline? _srpPipeline;

    [UnmanagedCallersOnly]
    public static void Bootstrap(IntPtr apiPtr)
    {
        ScriptEngine.Bootstrap(apiPtr);
        GameScripts.Generated.ScriptRegistry.RegisterAll();

        // 设置 SRP 渲染回调
        _srpPipeline = new ShaderpackPipeline();
        ScriptEngine.OnRenderCallback = OnSrpRender;

        // 初始化游戏世界
        var init = Node.Create("__PrismaCraftInit__");
        init.AddScript<PrismaCraftGame>();

        // SRP 端到端测试
        SRPTest.Run();

        Console.WriteLine("[PrismaCraft] Game initialized with SRP pipeline");
    }

    [UnmanagedCallersOnly]
    public static void OnFrame(float dt)
    {
        ScriptEngine.OnFrame(dt);
    }

    [UnmanagedCallersOnly]
    public static void OnRender(float dt)
    {
        ScriptEngine.OnRender(dt);
    }

    /// <summary>
    /// SRP 渲染回调：由 C++ BeginFrame/EndFrame 之间调用。
    /// </summary>
    private static void OnSrpRender(float dt)
    {
        // 更新 uniform
        if (_srpPipeline != null)
        {
            _srpPipeline.Uniforms.FrameTime = dt;
            _srpPipeline.Uniforms.FrameCounter++;
            _srpPipeline.Render();
        }
    }
}
