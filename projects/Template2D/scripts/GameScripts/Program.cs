namespace GameScripts;

/// <summary>
/// 自包含发布需要可执行入口点。
/// 实际初始化由 C++ 端通过 ScriptEntry.Bootstrap 函数指针触发。
/// </summary>
public static class Program
{
    static void Main(string[] args)
    {
        // 不由命令行运行，入口点仅为满足 dotnet publish --self-contained 要求
        _ = args;
    }
}
