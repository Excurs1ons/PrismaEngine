namespace Prisma.SRP;

public enum ShaderStage : uint
{
    Vertex = 0,
    Fragment = 1,
    Compute = 2,
    Geometry = 3,
}

public sealed class Shader : IDisposable
{
    public uint Handle { get; private set; }
    public ShaderStage Stage { get; }

    public Shader(string source, ShaderStage stage)
    {
        Stage = stage;
        unsafe
        {
            fixed (byte* srcPtr = System.Text.Encoding.UTF8.GetBytes(source))
            {
                Handle = Interop.API.SrpCreateShader(srcPtr, (uint)source.Length, (uint)stage);
            }
        }
    }

    public void Dispose()
    {
        if (Handle != 0)
        {
            unsafe { Interop.API.SrpDestroyShader(Handle); }
            Handle = 0;
        }
    }
}
