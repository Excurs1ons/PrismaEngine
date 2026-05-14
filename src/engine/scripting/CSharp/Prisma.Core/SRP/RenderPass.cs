namespace Prisma.SRP;

/// <summary>
/// 单个渲染 Pass。封装 GPU pipeline + uniform 绑定。
/// </summary>
public class RenderPass : IDisposable
{
    public string Name { get; }
    public uint VertexShader { get; }
    public uint FragmentShader { get; }
    public uint Pipeline { get; }
    public bool IsFinal { get; init; }
    public MinecraftUniforms Uniforms { get; }

    internal RenderPass(string name, uint vs, uint fs, uint pipeline, MinecraftUniforms uniforms)
    {
        Name = name;
        VertexShader = vs;
        FragmentShader = fs;
        Pipeline = pipeline;
        Uniforms = uniforms;
    }

    /// <summary>执行该 Pass。</summary>
    public virtual void Execute()
    {
        unsafe
        {
            Interop.API.SrpBeginFrame();
            // TODO: cmdBeginRenderPass + cmdBindPipeline + push uniforms + draw + cmdEndRenderPass
            // These command-buffer-level APIs will be added in the next iteration
            Interop.API.SrpEndFrame();
        }
    }

    public void Dispose()
    {
        unsafe
        {
            if (VertexShader != 0) Interop.API.SrpDestroyShader(VertexShader);
            if (FragmentShader != 0) Interop.API.SrpDestroyShader(FragmentShader);
            if (Pipeline != 0) Interop.API.SrpDestroyPipeline(Pipeline);
        }
    }
}
