using System;
using System.Buffers;
using System.Buffers.Text;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;

namespace Prisma;

/// <summary>
/// 调试日志工具类�?
/// Debug logging utility.
/// </summary>
public static class Debug
{
    private const int StackThreshold = 512;

    public static void Log(object? message) => SendToEngine("INFO", message?.ToString() ?? "null");
    public static void LogWarning(object? message) => SendToEngine("WARN", message?.ToString() ?? "null");
    public static void LogError(object? message) => SendToEngine("ERROR", message?.ToString() ?? "null");

    /// <summary> 高性能零分配插值字符串日志�?High-performance zero-allocation interpolated string logging. </summary>
    public static void Log(ref PrismaLogInterpolatedStringHandler handler) => SendHandlerToEngine("INFO", ref handler);
    public static void LogWarning(ref PrismaLogInterpolatedStringHandler handler) => SendHandlerToEngine("WARN", ref handler);
    public static void LogError(ref PrismaLogInterpolatedStringHandler handler) => SendHandlerToEngine("ERROR", ref handler);

    private static unsafe void SendHandlerToEngine(string tag, ref PrismaLogInterpolatedStringHandler handler)
    {
        ReadOnlySpan<byte> msgSpan = handler.WrittenSpan;
        SendRawToEngine(tag, msgSpan);
        handler.Dispose();
    }

    private static unsafe void SendRawToEngine(string tag, ReadOnlySpan<byte> msgSpan)
    {
        int tagByteCount = Encoding.UTF8.GetByteCount(tag);
        Span<byte> tagSpan = stackalloc byte[tagByteCount + 1];
        Encoding.UTF8.GetBytes(tag, tagSpan);
        tagSpan[tagByteCount] = 0;

        fixed (byte* tagPtr = tagSpan)
        fixed (byte* msgPtr = msgSpan)
        {
            Interop.API.Log(tagPtr, msgPtr);
        }
    }

    private static unsafe void SendToEngine(string tag, string message)
    {
        if (Interop.API.Log == null) { Console.WriteLine($"[{tag}] {message}"); return; }
        
        int msgByteCount = Encoding.UTF8.GetByteCount(message);
        byte[]? arrayPoolBuffer = null;
        Span<byte> msgSpan = msgByteCount < StackThreshold 
            ? stackalloc byte[msgByteCount + 1] 
            : (arrayPoolBuffer = ArrayPool<byte>.Shared.Rent(msgByteCount + 1));

        try
        {
            Encoding.UTF8.GetBytes(message, msgSpan);
            msgSpan[msgByteCount] = 0;
            SendRawToEngine(tag, msgSpan);
        }
        finally
        {
            if (arrayPoolBuffer != null) ArrayPool<byte>.Shared.Return(arrayPoolBuffer);
        }
    }
}

/// <summary>
/// 优化后的零分配插值字符串处理器�?
/// Optimized zero-allocation interpolated string handler.
/// </summary>
[InterpolatedStringHandler]
public ref struct PrismaLogInterpolatedStringHandler
{
    private const int MaxStackSize = 256;
    private Span<byte> _buffer;
    private int _pos;
    private byte[]? _arrayFromPool;

    public PrismaLogInterpolatedStringHandler(int literalLength, int formattedCount)
    {
        // 增加预估空间，并多留一个字节给 null 终止�?
        int initialSize = literalLength + formattedCount * 64;
        
        if (initialSize < MaxStackSize)
        {
            // �?C# 11+ 中，可以通过这种方式在构造函数中处理 stackalloc (实际上是调用处分�?
            // 但为了兼容性，我们这里使用一个预留的字段或让调用者传�?
            // 实际�?InterpolatedStringHandler 编译器生成的代码支持 stackalloc 分配缓冲区传进来
            // 这里我们先实现池化，但在构造函数里区分大小�?
            _arrayFromPool = null;
            // 注意：stackalloc 只能在方法内。构造函数无法持�?stackalloc �?span 除非它是 ref struct 且由外部传入�?
            // C# 编译器在处理 InterpolatedStringHandler 时，会尝试调用带 Span 的构造函数�?
            _arrayFromPool = ArrayPool<byte>.Shared.Rent(initialSize + 1);
            _buffer = _arrayFromPool;
        }
        else
        {
            _arrayFromPool = ArrayPool<byte>.Shared.Rent(initialSize + 1);
            _buffer = _arrayFromPool;
        }
        _pos = 0;
    }

    // 添加编译器优先选择的带 Span 的构造函数（用于 stackalloc 优化�?
    public PrismaLogInterpolatedStringHandler(int literalLength, int formattedCount, Span<byte> stackBuffer)
    {
        _arrayFromPool = null;
        _buffer = stackBuffer;
        _pos = 0;
    }

    public void AppendLiteral(string value)
    {
        // 简单扩容逻辑：如果空间不足，抛出异常。在高性能场景下，预估通常是准确的�?
        int bytesWritten = Encoding.UTF8.GetBytes(value, _buffer.Slice(_pos));
        _pos += bytesWritten;
    }

    public void AppendFormatted<T>(T value)
    {
        // 检查是否支�?ISpanFormattable (包括 Vector2, Vector3, Color)
        if (value is ISpanFormattable formattable)
        {
            Span<char> charBuffer = stackalloc char[128];
            if (formattable.TryFormat(charBuffer, out int charsWritten, default, null))
            {
                int bytesWritten = Encoding.UTF8.GetBytes(charBuffer.Slice(0, charsWritten), _buffer.Slice(_pos));
                _pos += bytesWritten;
                return;
            }
        }

        AppendLiteral(value?.ToString() ?? "null");
    }

    // 针对基本类型的极致优�?
    public void AppendFormatted(float value) { if (Utf8Formatter.TryFormat(value, _buffer.Slice(_pos), out int b)) _pos += b; }
    public void AppendFormatted(int value) { if (Utf8Formatter.TryFormat(value, _buffer.Slice(_pos), out int b)) _pos += b; }

    internal ReadOnlySpan<byte> WrittenSpan
    {
        get
        {
            _buffer[_pos] = 0; // 确保 null 终止
            return _buffer.Slice(0, _pos + 1);
        }
    }

    public void Dispose()
    {
        if (_arrayFromPool != null)
        {
            ArrayPool<byte>.Shared.Return(_arrayFromPool);
            _arrayFromPool = null;
        }
    }
}
