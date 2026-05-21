namespace Prisma.SRP;

/// <summary>
/// 引擎内置 2D 渲染管线（SRP 驱动）。
/// 所有 2D 项目共用此管线，通过 RendererFeature 注入项目特定的渲染逻辑。
/// 实际 2D 渲染由 C++ Renderer2D/Pipeline2D 执行，C# SRP 负责编排与注入。
/// </summary>
public sealed class SRP2DRenderPipeline : RenderPipeline
{
    public override void Build()
    {
    }
}
