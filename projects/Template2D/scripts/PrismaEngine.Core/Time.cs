namespace PrismaEngine;

/// <summary>
/// 时间静态 API。
/// </summary>
public static class Time
{
    /// <summary>上一帧的增量时间（秒）。</summary>
    public static float DeltaTime { get; internal set; }

    /// <summary>自场景启动以来的总时间（秒）。</summary>
    public static float Elapsed { get; internal set; }
}
