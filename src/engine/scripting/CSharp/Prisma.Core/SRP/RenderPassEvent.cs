namespace Prisma.SRP;

/// <summary>
/// 渲染 Pass 注入点。RendererFeature 通过此枚举指定在管线的哪个阶段执行。
/// </summary>
public enum RenderPassEvent
{
    BeforeRendering = 0,
    AfterSkybox = 100,
    BeforeGeometry = 200,
    AfterGeometry = 300,
    BeforeTransparents = 400,
    AfterTransparents = 500,
    BeforePostProcessing = 600,
    AfterPostProcessing = 700,
    AfterRendering = 800,
}
