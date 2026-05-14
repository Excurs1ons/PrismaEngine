namespace Prisma.SRP;

/// <summary>
/// 可编程渲染管线基类。C# 控制 Pass 顺序和执行逻辑。
/// 每帧 Build 被调用，生成当前帧的 Pass 列表并执行。
/// </summary>
public abstract class RenderPipeline
{
    protected List<RenderPass> passes = new();

    /// <summary>每帧构建 Pass 列表。子类实现具体的渲染管线。</summary>
    public abstract void Build();

    /// <summary>执行所有 Pass。</summary>
    public void Render()
    {
        unsafe { Interop.API.SrpBeginFrame(); }

        for (int i = 0; i < passes.Count; i++)
            passes[i].Execute();

        unsafe { Interop.API.SrpEndFrame(); }
    }

    /// <summary>释放所有 GPU 资源。</summary>
    public virtual void Dispose()
    {
        for (int i = 0; i < passes.Count; i++)
            passes[i].Dispose();
        passes.Clear();
    }
}

/// <summary>
/// Shaderpack 渲染管线 — 根据 Iris/OptiFine shaderpack 动态生成 Pass。
/// </summary>
public class ShaderpackPipeline : RenderPipeline
{
    private ShaderpackAsset? _pack;
    private MinecraftUniforms _uniforms = new();

    public void LoadPack(ShaderpackAsset pack)
    {
        _pack = pack;
        Build();
    }

    public MinecraftUniforms Uniforms => _uniforms;

    public override void Build()
    {
        // 清理旧 Pass
        for (int i = 0; i < passes.Count; i++)
            passes[i].Dispose();
        passes.Clear();

        if (_pack == null) return;

        var programs = _pack.Programs;

        // Shadow Pass
        if (programs.TryGetValue("shadow", out var shadow))
        {
            passes.Add(CompileGBufferPass("shadow", shadow));
        }

        // GBuffers terrain
        if (programs.TryGetValue("gbuffers_terrain", out var terrain))
        {
            passes.Add(CompileGBufferPass("gbuffers_terrain", terrain, 
                rtFormats: new uint[] { 0, 1, 2 })); // colortex0,1,2
        }

        // Deferred
        if (programs.TryGetValue("deferred", out var deferred))
        {
            passes.Add(CompileCompositePass("deferred", deferred));
        }

        // Composite chain (composite, composite1, composite2...)
        for (int i = 0; ; i++)
        {
            string name = i == 0 ? "composite" : $"composite{i}";
            if (programs.TryGetValue(name, out var comp))
                passes.Add(CompileCompositePass(name, comp));
            else
                break;
        }

        // Final output
        if (programs.TryGetValue("final", out var final))
        {
            passes.Add(CompileCompositePass("final", final, isFinal: true));
        }
    }

    private RenderPass CompileGBufferPass(string name, ShaderpackProgram program, uint[]? rtFormats = null)
    {
        var vs = CompileShader(program.VertexSource, SRPShaderStage.Vertex);
        var fs = CompileShader(program.FragmentSource, SRPShaderStage.Fragment);

        var desc = new SRPPipelineDesc
        {
            VertexShader = vs,
            FragmentShader = fs,
            NumRenderTargets = (uint)(rtFormats?.Length ?? 1),
            DepthStencilFormat = 50, // D32_Float
            CullMode = 2, // Back
            DepthTest = 1,
            DepthWrite = 1,
            DepthFunc = 4, // Less
            BlendColorWriteMask = 0xF,
        };

        if (rtFormats != null)
        {
            unsafe
            {
                for (int i = 0; i < rtFormats.Length && i < 8; i++)
                    desc.RenderTargetFormats[i] = rtFormats[i];
            }
        }

        uint pipeline;
        unsafe { pipeline = Interop.API.SrpCreatePipeline(&desc); }

        return new RenderPass(name, vs, fs, pipeline, _uniforms);
    }

    private RenderPass CompileCompositePass(string name, ShaderpackProgram program, bool isFinal = false)
    {
        var vs = CompileShader(program.VertexSource, SRPShaderStage.Vertex);
        var fs = CompileShader(program.FragmentSource, SRPShaderStage.Fragment);

        var desc = new SRPPipelineDesc
        {
            VertexShader = vs,
            FragmentShader = fs,
            NumRenderTargets = 1,
            CullMode = 0, // None (fullscreen quad)
            DepthTest = 0,
            DepthWrite = 0,
            BlendColorWriteMask = 0xF,
        };

        uint pipeline;
        unsafe { pipeline = Interop.API.SrpCreatePipeline(&desc); }

        return new RenderPass(name, vs, fs, pipeline, _uniforms) { IsFinal = isFinal };
    }

    private static uint CompileShader(string source, SRPShaderStage stage)
    {
        unsafe
        {
            fixed (byte* srcPtr = System.Text.Encoding.UTF8.GetBytes(source))
            {
                return Interop.API.SrpCreateShader(srcPtr, (uint)source.Length, (uint)stage);
            }
        }
    }
}

internal enum SRPShaderStage : uint
{
    Vertex = 0,
    Fragment = 1,
    Compute = 2,
    Geometry = 3
}
