using System;
using System.Collections.Generic;

namespace Prisma;

public class AudioGraph : IDisposable
{
    private ulong _handle;
    private bool _disposed;
    private readonly List<DSPNode> _nodes = new();

    public AudioGraph(uint sampleRate = 48000, uint framesPerBlock = 256)
    {
        unsafe
        {
            _handle = Interop.API.AudioCreateGraph(sampleRate, framesPerBlock);
        }
        if (_handle == 0)
            throw new InvalidOperationException("Failed to create AudioGraph");
    }

    public DSPNode CreateNode(string nodeType, string name = "")
    {
        if (_disposed)
            throw new ObjectDisposedException(nameof(AudioGraph));
        if (string.IsNullOrEmpty(nodeType))
            throw new ArgumentNullException(nameof(nodeType));

        unsafe
        {
            byte[] typeBytes = Interop.StringToUtf8(nodeType);
            byte[] nameBytes = Interop.StringToUtf8(name);

            fixed (byte* pType = typeBytes)
            fixed (byte* pName = nameBytes)
            {
                ulong nodeHandle = Interop.API.AudioGraphCreateNode(_handle, pType, pName);
                if (nodeHandle == 0)
                    throw new InvalidOperationException($"Failed to create node: {nodeType}");

                var node = new DSPNode(nodeHandle, this);
                _nodes.Add(node);
                return node;
            }
        }
    }

    public bool Connect(DSPNode source, string sourcePin, DSPNode target, string inputPin)
    {
        if (_disposed)
            throw new ObjectDisposedException(nameof(AudioGraph));
        if (source == null) throw new ArgumentNullException(nameof(source));
        if (target == null) throw new ArgumentNullException(nameof(target));
        if (sourcePin == null) throw new ArgumentNullException(nameof(sourcePin));
        if (inputPin == null) throw new ArgumentNullException(nameof(inputPin));

        unsafe
        {
            byte[] srcPinBytes = Interop.StringToUtf8(sourcePin);
            byte[] dstPinBytes = Interop.StringToUtf8(inputPin);

            fixed (byte* pSrcPin = srcPinBytes)
            fixed (byte* pDstPin = dstPinBytes)
            {
                return Interop.API.AudioGraphConnect(_handle, source.Handle, pSrcPin, target.Handle, pDstPin);
            }
        }
    }

    public bool Disconnect(DSPNode source, DSPNode target)
    {
        if (_disposed)
            throw new ObjectDisposedException(nameof(AudioGraph));
        if (source == null) throw new ArgumentNullException(nameof(source));
        if (target == null) throw new ArgumentNullException(nameof(target));

        unsafe
        {
            return Interop.API.AudioGraphDisconnect(_handle, source.Handle, target.Handle);
        }
    }

    public void RemoveNode(DSPNode node)
    {
        if (_disposed)
            throw new ObjectDisposedException(nameof(AudioGraph));
        if (node == null) throw new ArgumentNullException(nameof(node));

        unsafe
        {
            Interop.API.AudioGraphRemoveNode(_handle, node.Handle);
        }
        node.Invalidate();
        _nodes.Remove(node);
    }

    public void Dispose()
    {
        if (_disposed) return;
        _disposed = true;

        foreach (var node in _nodes)
            node.Invalidate();
        _nodes.Clear();

        unsafe
        {
            Interop.API.AudioDestroyGraph(_handle);
        }
        _handle = 0;
    }

    internal ulong Handle => _handle;
    internal bool IsDisposed => _disposed;
}
