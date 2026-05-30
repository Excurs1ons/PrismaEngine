using System;
using System.Runtime.InteropServices;
using System.IO;

namespace Prisma;

/// <summary>
/// 资产管理器。提供从 C++ 引擎（包括 Android APK）读取原始数据的能力。
/// Asset Manager. Provides capability to read raw data from C++ engine (including Android APK).
/// </summary>
public static class AssetManager
{
    /// <summary>
    /// 从资源路径读取二进制数据。在 Android 上透明支持 APK 资产读取。
    /// Reads binary data from an asset path. Transparently supports APK asset reading on Android.
    /// </summary>
    public static unsafe byte[] ReadBinary(string path)
    {
        nuint size = 0;
        byte* pathPtr = null;
        
        fixed (byte* p = Interop.StringToUtf8(path))
        {
            pathPtr = p;
        }

        void* data = Interop.API.ReadAssetData(pathPtr, &size);
        if (data == null || size == 0)
        {
            return Array.Empty<byte>();
        }

        byte[] result = new byte[(long)size];
        Marshal.Copy((IntPtr)data, result, 0, (int)size);
        
        Interop.API.FreeAssetData(data);
        return result;
    }

    /// <summary>
    /// 从资源路径读取文本数据。
    /// Reads text data from an asset path.
    /// </summary>
    public static string ReadText(string path)
    {
        var data = ReadBinary(path);
        if (data.Length == 0) return string.Empty;
        return System.Text.Encoding.UTF8.GetString(data);
    }
}
