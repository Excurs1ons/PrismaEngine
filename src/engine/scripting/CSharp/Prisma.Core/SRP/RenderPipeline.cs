namespace Prisma.SRP;

public abstract class RenderPipeline
{
    protected List<RenderPass> passes = new();
    protected List<RendererFeature> features = new();

    public abstract void Build();

    public void AddFeature(RendererFeature feature)
    {
        feature.Create();
        features.Add(feature);
    }

    public void RemoveFeature(RendererFeature feature)
    {
        features.Remove(feature);
        feature.Dispose();
    }

    public void Render()
    {
        var cmd = new CommandBuffer();
        cmd.BeginFrame();

        var sorted = CollectAndSort();

        foreach (var entry in sorted)
        {
            entry(cmd);
        }

        cmd.EndFrame();
    }

    private struct ScheduledPass
    {
        public RenderPassEvent Event;
        public Action<CommandBuffer> Execute;
    }

    private List<Action<CommandBuffer>> CollectAndSort()
    {
        var scheduled = new List<ScheduledPass>();

        for (int i = 0; i < passes.Count; i++)
        {
            var pass = passes[i];
            scheduled.Add(new ScheduledPass
            {
                Event = RenderPassEvent.AfterSkybox,
                Execute = cmd =>
                {
                    cmd.BeginRenderPass();
                    pass.Execute(cmd);
                    cmd.EndRenderPass();
                }
            });
        }

        for (int i = 0; i < features.Count; i++)
        {
            var feature = features[i];
            scheduled.Add(new ScheduledPass
            {
                Event = feature.InjectionPoint,
                Execute = feature.Execute
            });
        }

        scheduled.Sort((a, b) => a.Event.CompareTo(b.Event));

        var result = new List<Action<CommandBuffer>>(scheduled.Count);
        for (int i = 0; i < scheduled.Count; i++)
            result.Add(scheduled[i].Execute);
        return result;
    }

    public virtual void Dispose()
    {
        for (int i = 0; i < passes.Count; i++)
            passes[i].Dispose();
        for (int i = 0; i < features.Count; i++)
            features[i].Dispose();
        passes.Clear();
        features.Clear();
    }
}

public class ShaderpackPipeline : RenderPipeline
{
    private ShaderpackAsset? _pack;

    public void LoadPack(ShaderpackAsset pack)
    {
        _pack = pack;
        Build();
    }

    public override void Build()
    {
        for (int i = 0; i < passes.Count; i++)
            passes[i].Dispose();
        passes.Clear();

        if (_pack == null) return;

        var programs = _pack.Programs;

        if (programs.TryGetValue("shadow", out var shadow))
            passes.Add(MakeGBufferPass("shadow", shadow));

        if (programs.TryGetValue("gbuffers_terrain", out var terrain))
            passes.Add(MakeGBufferPass("gbuffers_terrain", terrain));

        if (programs.TryGetValue("deferred", out var deferred))
            passes.Add(MakeCompositePass("deferred", deferred));

        for (int i = 0; ; i++)
        {
            string name = i == 0 ? "composite" : $"composite{i}";
            if (programs.TryGetValue(name, out var comp))
                passes.Add(MakeCompositePass(name, comp));
            else
                break;
        }

        if (programs.TryGetValue("final", out var final))
            passes.Add(MakeCompositePass("final", final, isFinal: true));
    }

    private RenderPass MakeGBufferPass(string name, ShaderpackProgram program)
    {
        var vs = new Shader(program.VertexSource, ShaderStage.Vertex);
        var fs = new Shader(program.FragmentSource, ShaderStage.Fragment);
        var pipeline = new GraphicsPipeline(vs, fs, new PipelineDesc
        {
            NumRenderTargets = 1,
            DepthStencilFormat = 50,
            CullMode = 2,
            DepthTest = true,
            DepthWrite = true,
            DepthFunc = 4,
            BlendColorWriteMask = 0xF,
        });
        return new RenderPass(name, vs, fs, pipeline);
    }

    private RenderPass MakeCompositePass(string name, ShaderpackProgram program, bool isFinal = false)
    {
        var vs = new Shader(program.VertexSource, ShaderStage.Vertex);
        var fs = new Shader(program.FragmentSource, ShaderStage.Fragment);
        var pipeline = new GraphicsPipeline(vs, fs, new PipelineDesc
        {
            NumRenderTargets = 1,
            CullMode = 0,
            DepthTest = false,
            DepthWrite = false,
            BlendColorWriteMask = 0xF,
        });
        return new RenderPass(name, vs, fs, pipeline) { IsFinal = isFinal };
    }
}
