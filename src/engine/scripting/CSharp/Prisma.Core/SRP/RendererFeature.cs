namespace Prisma.SRP;

/// <summary>
/// 渲染特性基类。继承此类并注册到 RenderPipeline，可在管线的任意阶段插入自定义渲染逻辑。
/// 类似 Unity ScriptableRendererFeature。
/// </summary>
public abstract class RendererFeature : IDisposable
{
    public string Name { get; set; } = "";
    public RenderPassEvent InjectionPoint { get; set; } = RenderPassEvent.BeforeRendering;

    /// <summary>添加特性时调用一次。在此创建 GPU 资源。</summary>
    public virtual void Create() { }

    /// <summary>在 InjectionPoint 阶段每帧调用。录制渲染/计算命令到此 cmd。</summary>
    public abstract void Execute(CommandBuffer cmd);

    public virtual void Dispose() { }
}
