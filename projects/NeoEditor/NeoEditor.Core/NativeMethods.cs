using System.Runtime.InteropServices;

namespace NeoEditor.Core.Interop;

internal static class NativeMethods
{
    [DllImport("kernel32", CharSet = CharSet.Ansi, ExactSpelling = true, SetLastError = true)]
    internal static extern IntPtr LoadLibrary(string lpFileName);

    [DllImport("kernel32", CharSet = CharSet.Ansi, ExactSpelling = true)]
    internal static extern IntPtr GetProcAddress(IntPtr hModule, string lpProcName);

    [DllImport("kernel32", ExactSpelling = true, SetLastError = true)]
    internal static extern bool FreeLibrary(IntPtr hModule);
}
