using System.Runtime.InteropServices;

namespace NeoEditor.Core.Interop;

[StructLayout(LayoutKind.Sequential)]
internal struct EngineCreateInfo
{
    public IntPtr Name;
    public int    Headless;
    public int    MinLogLevel;
    public int    PresentMode;
    public uint   MaxFPS;
    public int    RefreshAssetDatabaseOnStartup;
}

[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
internal delegate IntPtr CreateInterfaceDelegate(IntPtr name, IntPtr param);

[UnmanagedFunctionPointer(CallingConvention.ThisCall)]
internal delegate int EngineInitializeDelegate(IntPtr enginePtr);

[UnmanagedFunctionPointer(CallingConvention.ThisCall)]
internal delegate int EngineRunDelegate(IntPtr enginePtr, IntPtr pluginPath);

[UnmanagedFunctionPointer(CallingConvention.ThisCall)]
internal delegate void EngineShutdownDelegate(IntPtr enginePtr);

internal sealed unsafe class EngineHandle : IDisposable
{
    private IntPtr _modulePtr;
    private IntPtr _enginePtr;
    private bool   _disposed;

    private EngineInitializeDelegate? _initialize;
    private EngineRunDelegate?        _run;
    private EngineShutdownDelegate?   _shutdown;

    public IntPtr RawPtr => _enginePtr;

    public EngineHandle(string dllPath, EngineCreateInfo config)
    {
        _modulePtr = NativeMethods.LoadLibrary(dllPath);
        if (_modulePtr == IntPtr.Zero)
        {
            int err = Marshal.GetLastWin32Error();
            throw new InvalidOperationException(
                $"Failed to load engine DLL '{dllPath}'. Win32 error: {err}");
        }

        IntPtr createInterfacePtr;
        try
        {
            createInterfacePtr = NativeMethods.GetProcAddress(_modulePtr, "CreateInterface");
        }
        catch
        {
            NativeMethods.FreeLibrary(_modulePtr);
            _modulePtr = IntPtr.Zero;
            throw new InvalidOperationException(
                "Missing 'CreateInterface' export in engine DLL.");
        }

        if (createInterfacePtr == IntPtr.Zero)
        {
            NativeMethods.FreeLibrary(_modulePtr);
            _modulePtr = IntPtr.Zero;
            throw new InvalidOperationException(
                "Missing 'CreateInterface' export in engine DLL.");
        }

        var createInterface =
            Marshal.GetDelegateForFunctionPointer<CreateInterfaceDelegate>(createInterfacePtr);

        EngineCreateInfo localConfig = config;
        IntPtr interfaceName = Marshal.StringToCoTaskMemAnsi("IEngine");

        try
        {
            _enginePtr = createInterface(interfaceName, (IntPtr)(&localConfig));
        }
        finally
        {
            Marshal.FreeCoTaskMem(interfaceName);
        }

        if (_enginePtr == IntPtr.Zero)
        {
            NativeMethods.FreeLibrary(_modulePtr);
            _modulePtr = IntPtr.Zero;
            throw new InvalidOperationException(
                "CreateInterface returned null for 'IEngine'.");
        }

        CacheVtableDelegates();
    }

    // x64 vtable: [0]=~IEngine [1]=SetCommandLine [2]=Initialize [3]=Run [4]=Shutdown
    private void CacheVtableDelegates()
    {
        IntPtr* vtablePtr = (IntPtr*)_enginePtr;
        IntPtr* vtable    = (IntPtr*)*vtablePtr;

        _initialize = Marshal.GetDelegateForFunctionPointer<EngineInitializeDelegate>(vtable[2]);
        _run        = Marshal.GetDelegateForFunctionPointer<EngineRunDelegate>(vtable[3]);
        _shutdown   = Marshal.GetDelegateForFunctionPointer<EngineShutdownDelegate>(vtable[4]);
    }

    public int Initialize()
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        return _initialize!(_enginePtr);
    }

    public int Run(IntPtr pluginPath)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        return _run!(_enginePtr, pluginPath);
    }

    public void Shutdown()
    {
        if (_disposed) return;
        _shutdown!(_enginePtr);
    }

    public void Dispose()
    {
        if (_disposed) return;
        _disposed = true;

        if (_enginePtr != IntPtr.Zero)
        {
            Shutdown();
            _enginePtr = IntPtr.Zero;
        }

        if (_modulePtr != IntPtr.Zero)
        {
            NativeMethods.FreeLibrary(_modulePtr);
            _modulePtr = IntPtr.Zero;
        }
    }
}
