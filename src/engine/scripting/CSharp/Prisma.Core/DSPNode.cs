using System;

namespace Prisma;

public class DSPNode : IDisposable
{
    private ulong _handle;
    private AudioGraph _graph;
    private bool _disposed;

    internal DSPNode(ulong handle, AudioGraph graph)
    {
        _handle = handle;
        _graph = graph;
    }

    public string Name
    {
        get
        {
            if (_disposed)
                throw new ObjectDisposedException(nameof(DSPNode));

            unsafe
            {
                byte* namePtr = Interop.API.AudioNodeGetName(_handle);
                return Interop.Utf8ToString(namePtr);
            }
        }
        set
        {
            if (_disposed)
                throw new ObjectDisposedException(nameof(DSPNode));
            if (value == null)
                throw new ArgumentNullException(nameof(value));

            unsafe
            {
                byte[] nameBytes = Interop.StringToUtf8(value);
                fixed (byte* pName = nameBytes)
                {
                    Interop.API.AudioNodeSetName(_handle, pName);
                }
            }
        }
    }

    public void SetParameter(string name, float value)
    {
        if (_disposed)
            throw new ObjectDisposedException(nameof(DSPNode));
        if (name == null)
            throw new ArgumentNullException(nameof(name));

        unsafe
        {
            byte[] nameBytes = Interop.StringToUtf8(name);
            fixed (byte* pName = nameBytes)
            {
                Interop.API.AudioNodeSetParam(_handle, pName, value);
            }
        }
    }

    public float GetParameter(string name)
    {
        if (_disposed)
            throw new ObjectDisposedException(nameof(DSPNode));
        if (name == null)
            throw new ArgumentNullException(nameof(name));

        unsafe
        {
            byte[] nameBytes = Interop.StringToUtf8(name);
            fixed (byte* pName = nameBytes)
            {
                return Interop.API.AudioNodeGetParam(_handle, pName);
            }
        }
    }

    public void Dispose()
    {
        if (_disposed) return;
        _disposed = true;

        unsafe { Interop.API.AudioNodeDestroy(_handle); }
        _handle = 0;
        _graph = null;
    }

    internal void Invalidate()
    {
        _disposed = true;
        _handle = 0;
        _graph = null;
    }

    internal ulong Handle => _handle;
    internal bool IsDisposed => _disposed;
}
