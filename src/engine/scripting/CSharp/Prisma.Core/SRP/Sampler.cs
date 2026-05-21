namespace Prisma.SRP;

public sealed class Sampler : IDisposable
{
    public uint Handle { get; private set; }

    public Sampler(
        uint minFilter = 1, uint magFilter = 1, uint mipFilter = 1,
        uint addressU = 0, uint addressV = 0, uint addressW = 0)
    {
        var desc = new SRPSamplerDesc
        {
            MinFilter = minFilter,
            MagFilter = magFilter,
            MipFilter = mipFilter,
            AddressU = addressU,
            AddressV = addressV,
            AddressW = addressW,
        };
        unsafe
        {
            Handle = Interop.API.SrpCreateSampler(&desc);
        }
    }

    public void Dispose()
    {
        if (Handle != 0)
        {
            unsafe { Interop.API.SrpDestroySampler(Handle); }
            Handle = 0;
        }
    }
}
