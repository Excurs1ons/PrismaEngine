namespace Prisma.SRP;

public sealed class Texture : IDisposable
{
    public uint Handle { get; private set; }
    public int Width { get; }
    public int Height { get; }

    public Texture(int width, int height, uint format, byte[]? pixels = null)
    {
        Width = width;
        Height = height;
        unsafe
        {
            fixed (byte* p = pixels)
            {
                Handle = Interop.API.SrpCreateTexture2D(width, height, format, p, (uint)(pixels?.Length ?? 0));
            }
        }
    }

    public void Dispose()
    {
        if (Handle != 0)
        {
            unsafe { Interop.API.SrpDestroyTexture(Handle); }
            Handle = 0;
        }
    }
}
