using System;
using System.Runtime.InteropServices;
using System.Collections.Generic;

namespace Prisma;

/// <summary>
/// 跨平台文件系统接口。在 Android 上透明支持 APK 资产读取。
/// Cross-platform FileSystem interface. Transparently supports APK asset reading on Android.
/// </summary>
public static class FileSystem
{
    /// <summary>
    /// 检查文件是否存在。
    /// Checks if a file exists.
    /// </summary>
    public static unsafe bool Exists(string path)
    {
        fixed (byte* p = Interop.StringToUtf8(path))
        {
            nuint size = 0;
            void* data = Interop.API.ReadAssetData(p, &size);
            if (data != null)
            {
                Interop.API.FreeAssetData(data);
                return true;
            }
        }
        return false;
    }

    /// <summary>
    /// 读取二进制文件。
    /// Reads a binary file.
    /// </summary>
    public static byte[] ReadAllBytes(string path)
    {
        return AssetManager.ReadBinary(path);
    }

    /// <summary>
    /// 读取文本文件。
    /// Reads a text file.
    /// </summary>
    public static string ReadAllText(string path)
    {
        return AssetManager.ReadText(path);
    }
}
