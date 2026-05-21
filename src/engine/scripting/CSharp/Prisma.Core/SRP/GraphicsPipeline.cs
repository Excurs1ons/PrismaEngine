namespace Prisma.SRP;

public sealed class GraphicsPipeline : IDisposable
{
    public uint Handle { get; private set; }

    public GraphicsPipeline(Shader vertex, Shader fragment, in PipelineDesc desc)
    {
        var nativeDesc = new SRPPipelineDesc
        {
            VertexShader = vertex.Handle,
            FragmentShader = fragment.Handle,
            NumRenderTargets = desc.NumRenderTargets,
            DepthStencilFormat = desc.DepthStencilFormat,
            SampleCount = desc.SampleCount,
            CullMode = desc.CullMode,
            DepthTest = desc.DepthTest ? (byte)1 : (byte)0,
            DepthWrite = desc.DepthWrite ? (byte)1 : (byte)0,
            DepthFunc = desc.DepthFunc,
            BlendEnable = desc.BlendEnable ? (byte)1 : (byte)0,
            BlendColorWriteMask = desc.BlendColorWriteMask,
            Topology = desc.Topology,
        };
        if (desc.RenderTargetFormats != null)
            for (int i = 0; i < desc.RenderTargetFormats.Length && i < 8; i++)
                nativeDesc.RenderTargetFormats[i] = desc.RenderTargetFormats[i];

        unsafe
        {
            Handle = Interop.API.SrpCreatePipeline(&nativeDesc);
        }
    }

    public void Dispose()
    {
        if (Handle != 0)
        {
            unsafe { Interop.API.SrpDestroyPipeline(Handle); }
            Handle = 0;
        }
    }
}

public struct PipelineDesc
{
    public uint NumRenderTargets;
    public uint[]? RenderTargetFormats;
    public uint DepthStencilFormat;
    public uint SampleCount;
    public uint CullMode;
    public bool DepthTest;
    public bool DepthWrite;
    public uint DepthFunc;
    public bool BlendEnable;
    public byte BlendColorWriteMask;
    public uint Topology;
}
