using System;

namespace GameScripts;

/// <summary>
/// 存根入口点，满足 self-contained publish 需要 Exe 输出类型。
/// C++ 通过 CoreCLHoard 直接调用 [UnmanagedCallersOnly] 入口点。
/// </summary>
public class Program
{
    public static void Main()
    {
        // 不由 .NET 运行时启动，C++ CoreCLR 直接托管。
        // 此 Main 仅用于满足 .NET 工具链要求。
    }
}
