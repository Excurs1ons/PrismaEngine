namespace SRP2D;

using Prisma.SRP;

/// <summary>
/// SRP 驱动的 2D 渲染管线。
/// RendererFeature 可在任意 RenderPassEvent 注入自定义渲染。
/// 实际 2D 渲染由 C++ Renderer2D/Pipeline2D 执行，C# SRP 负责编排与注入。
/// </summary>
public sealed class SRP2DRenderPipeline : RenderPipeline
{
    public override void Build()
    {
    }
}
