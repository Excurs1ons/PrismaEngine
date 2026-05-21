namespace Prisma.SRP;

public sealed class ComputePipeline : IDisposable
{
    public uint Handle { get; private set; }

    public ComputePipeline(Shader shader, uint pushConstantSize = 0)
    {
        unsafe
        {
            Handle = Interop.API.SrpCreateComputePipeline(shader.Handle, pushConstantSize);
        }
    }

    public void Dispose()
    {
        if (Handle != 0)
        {
            unsafe { Interop.API.SrpDestroyComputePipeline(Handle); }
            Handle = 0;
        }
    }
}
