namespace PrismaEngine;

/// <summary>
/// 时间上下文信息。
/// Time context information.
/// </summary>
public class TimeContext
{
    public float DeltaTime { get; internal set; }
    public float Elapsed { get; internal set; }
    public float UnscaledDeltaTime { get; internal set; }
    public float TimeScale { get; set; } = 1.0f;
}

/// <summary>
/// 静态快捷访问。重定向到当前活跃世界。
/// Static shortcuts. Redirects to current active world.
/// </summary>
public static class Time
{
    // Cherno Optimization: 使用内部静态字段直接存储，避免每帧数万次的属性链访问和空检查。
    internal static float _deltaTime;
    internal static float _elapsed;
    internal static float _timeScale = 1.0f;
    
    // Cherno Fix: 标记是否处于并行更新状态，防止在并行期间修改全局状态导致的 Race Condition。
    internal static bool _isParallelStep;

    /// <summary> 上一帧增量时间。 Delta time. </summary>
    public static float DeltaTime => _deltaTime;
    /// <summary> 总运行时间。 Total elapsed time. </summary>
    public static float Elapsed => _elapsed;
    /// <summary> 时间缩放。 Time scale. </summary>
    public static float TimeScale 
    { 
        get => _timeScale; 
        set 
        { 
            if (_isParallelStep)
            {
                // 在并行期间修改全局状态是危险的！
                // 暂时记录错误，实际应用中可以抛出异常。
                Debug.LogError("Dangerous: Modification of TimeScale during parallel update! This will cause Race Conditions.");
                return;
            }
            _timeScale = value;
            if (World.Active != null) World.Active.Time.TimeScale = value; 
        } 
    }
}
