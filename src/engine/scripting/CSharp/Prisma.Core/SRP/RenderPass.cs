namespace Prisma.SRP;

public class RenderPass : IDisposable
{
    public string Name { get; }
    public Shader VertexShader { get; }
    public Shader FragmentShader { get; }
    public GraphicsPipeline Pipeline { get; }
    public bool IsFinal { get; init; }

    internal RenderPass(string name, Shader vs, Shader fs, GraphicsPipeline pipeline)
    {
        Name = name;
        VertexShader = vs;
        FragmentShader = fs;
        Pipeline = pipeline;
    }

    public virtual void Execute(CommandBuffer cmd)
    {
        cmd.BindPipeline(Pipeline);
        cmd.DrawFullScreenQuad();
    }

    public void Dispose()
    {
        VertexShader.Dispose();
        FragmentShader.Dispose();
        Pipeline.Dispose();
    }
}
