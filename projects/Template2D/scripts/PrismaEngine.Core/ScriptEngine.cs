using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace PrismaEngine;

/// <summary>
/// 内部脚本引擎。管理所有 Node 和 Script 的生命周期。
/// 普通 C# 方法，无 [UnmanagedCallersOnly] 限制。
/// </summary>
internal static class ScriptEngine
{
    private static List<Node> _allNodes = new();

    internal static void RegisterNode(Node n) => _allNodes.Add(n);

    /// <summary>
    /// C++ 端调用的引导入口。
    /// 接收 PrismaAPI 函数指针表，完成 C# 侧初始化。
    /// </summary>
    internal static void Bootstrap(IntPtr apiPtr)
    {
        unsafe { NativeAPI.Init((PrismaAPI*)apiPtr); }
    }

    /// <summary>
    /// C++ 端每帧调用的更新入口。
    /// </summary>
    internal static void OnFrame(float dt)
    {
        Time.DeltaTime = dt;
        Time.Elapsed += dt;

        foreach (var n in _allNodes)
        {
            if (n._destroyed) continue;

            for (int i = n._scripts.Count - 1; i >= 0; i--)
            {
                var s = n._scripts[i];
                if (!s._started)
                {
                    s.OnStart();
                    s._started = true;
                }
                s.OnUpdate(dt);
            }
        }
    }
}
