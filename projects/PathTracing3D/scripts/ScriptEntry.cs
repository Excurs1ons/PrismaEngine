using System;
using System.Runtime.InteropServices;
using Prisma;

namespace PathTracing3D;

/// <summary>
/// C++ 引擎通过 CoreCLR 调用的入口点。
/// Bootstrap: 初始化脚本引擎、注册脚本、创建摄像机控制器。
/// 场景由 engine 的 project.jsonc 反序列化加载，C# 仅处理交互逻辑。
/// </summary>
internal static class ScriptEntry
{
    [UnmanagedCallersOnly]
    public static void Bootstrap(IntPtr apiPtr)
    {
        ScriptEngine.Bootstrap(apiPtr);

        // 注册所有源码生成的脚本 (CameraController3D 的 TypeId 等)
        PathTracing3D.Generated.ScriptRegistry.RegisterAll();

        // 创建专用的控制器 Node（C# SoA ECS 层），挂载摄像机控制脚本。
        // 该节点与 C++ Scene 中的摄像机无关，仅通过 Interop.API 控制 3D 摄像机。
        var camCtrl = Node.Create("__CameraController__");
        camCtrl.AddScript<CameraController3D>();

        System.Console.WriteLine("[PathTracing3D] Scripts initialized, CameraController3D active");
    }

    [UnmanagedCallersOnly]
    public static void OnFrame(float dt)
    {
        ScriptEngine.OnFrame(dt);
    }
}
