namespace Prisma.SRP;

public sealed class Buffer : IDisposable
{
    public uint Handle { get; private set; }

    public Buffer(byte[] data, uint stride, bool isVertex)
    {
        unsafe
        {
            fixed (byte* p = data)
            {
                Handle = isVertex
                    ? Interop.API.SrpCreateVertexBuffer(p, (uint)data.Length, stride)
                    : Interop.API.SrpCreateIndexBuffer(p, (uint)data.Length, stride == 4 ? 1 : 0);
            }
        }
    }

    public void Dispose()
    {
        if (Handle != 0)
        {
            unsafe { Interop.API.SrpDestroyBuffer(Handle); }
            Handle = 0;
        }
    }
}
