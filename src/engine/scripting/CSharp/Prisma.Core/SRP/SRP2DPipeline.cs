namespace Prisma.SRP;

/// <summary>
/// 引擎内置 2D 渲染管线（SRP 驱动）。
/// 所有 2D 项目共用此管线。
/// 实际 2D 渲染由 C++ Renderer2D/Pipeline2D 执行，C# SRP 负责编排与注入。
/// 仅接受 RendererFeature2D 子类，编译期类型约束防止误注册 3D Feature。
/// </summary>
public sealed class SRP2DRenderPipeline : RenderPipeline
{
    public SSRWaterFeature2D WaterSSR { get; } = new();

    public override void Build()
    {
        AddFeature(WaterSSR);
    }

    public void AddFeature(RendererFeature2D feature) => base.AddFeature(feature);
}
