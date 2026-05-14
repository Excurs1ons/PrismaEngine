namespace Prisma.SRP;

/// <summary>
/// 单个渲染 Pass。封装 GPU pipeline + uniform 绑定 + 命令录制。
/// </summary>
public class RenderPass : IDisposable
{
    public string Name { get; }
    public uint VertexShader { get; }
    public uint FragmentShader { get; }
    public uint Pipeline { get; }
    public bool IsFinal { get; init; }
    public MinecraftUniforms Uniforms { get; }

    // 渲染目标句柄（由管线分配）
    internal uint ColorRT;
    internal uint DepthRT;

    internal RenderPass(string name, uint vs, uint fs, uint pipeline, MinecraftUniforms uniforms)
    {
        Name = name;
        VertexShader = vs;
        FragmentShader = fs;
        Pipeline = pipeline;
        Uniforms = uniforms;
    }

    /// <summary>
    /// 录制并提交该 Pass 的渲染命令。
    /// C# SRP 控制每个 Pass 的完整命令流。
    /// </summary>
    public virtual void Execute()
    {
        unsafe
        {
            // 绑定管线
            Interop.API.SrpCmdBindPipeline(Pipeline);

            // Viewport 覆盖全屏
            int w = 1280, h = 720; // TODO: get from engine
            Interop.API.SrpCmdSetViewport(0, 0, w, h);
            Interop.API.SrpCmdSetScissor(0, 0, w, h);

            // 对 composite-style Pass：绘制全屏四边形
            // 对 gbuffers-style Pass：后续会绑定 VB/IB + 绘制区块
            if (Name.StartsWith("composite") || Name == "final" || Name == "deferred")
            {
                // Push uniforms
                PushUniforms();
                Interop.API.SrpCmdDrawFullScreenQuad();
            }
            // gbuffers/shadow Pass 由外部额外调用 DrawChunk 系列命令
        }
    }

    /// <summary>推送 Minecraft uniform 数据到 GPU。</summary>
    protected virtual void PushUniforms()
    {
        // TODO: 填充 UBO / push constants
        // 通过 Interop.API.SrpCmdPushConstants(...) 或描述符集更新
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
