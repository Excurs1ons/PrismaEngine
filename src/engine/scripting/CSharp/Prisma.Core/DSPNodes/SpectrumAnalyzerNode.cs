using System;
using System.Runtime.InteropServices;

namespace Prisma;

public class SpectrumAnalyzerNode : IDisposable
{
    private ulong _handle;
    private bool _disposed;

    public SpectrumAnalyzerNode(uint fftSize = 2048)
    {
        _handle = Interop.API.AudioCreateSpectrumAnalyzer(fftSize);
        if (_handle == 0)
            throw new InvalidOperationException("Failed to create SpectrumAnalyzer");
    }

    public void Process(float[] input, uint sampleRate)
    {
        if (_disposed) throw new ObjectDisposedException(nameof(SpectrumAnalyzerNode));
        unsafe
        {
            fixed (float* pInput = input)
            {
                Interop.API.AudioSpectrumProcessFloats(_handle, pInput, (uint)input.Length, sampleRate);
            }
        }
    }

    public struct SpectrumBin
    {
        public float Frequency;
        public float Magnitude;  // dB
        public float Phase;      // radians
    }

    public SpectrumBin[] GetSpectrum()
    {
        if (_disposed) throw new ObjectDisposedException(nameof(SpectrumAnalyzerNode));

        unsafe
        {
            // First call with null to get count
            uint count = Interop.API.AudioSpectrumGetBins(_handle, null, null, null, 0);
            if (count == 0) return Array.Empty<SpectrumBin>();

            var freq = new float[count];
            var mag = new float[count];
            var phase = new float[count];

            fixed (float* pFreq = freq)
            fixed (float* pMag = mag)
            fixed (float* pPhase = phase)
            {
                uint actual = Interop.API.AudioSpectrumGetBins(_handle, pFreq, pMag, pPhase, count);

                var result = new SpectrumBin[actual];
                for (int i = 0; i < actual; i++)
                {
                    result[i] = new SpectrumBin
                    {
                        Frequency = freq[i],
                        Magnitude = mag[i],
                        Phase = phase[i]
                    };
                }
                return result;
            }
        }
    }

    public float PeakMagnitude
    {
        get
        {
            if (_disposed) throw new ObjectDisposedException(nameof(SpectrumAnalyzerNode));
            return Interop.API.AudioSpectrumGetPeak(_handle);
        }
    }

    public void Dispose()
    {
        if (_disposed) return;
        _disposed = true;
        Interop.API.AudioDestroySpectrumAnalyzer(_handle);
        _handle = 0;
    }
}
