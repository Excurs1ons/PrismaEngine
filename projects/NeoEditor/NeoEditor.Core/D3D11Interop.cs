using System.Runtime.InteropServices;

namespace NeoEditor.Core.Interop;

// ============================================================================
// COM Method Delegates (for manual vtable-based dispatch)
// ============================================================================

[UnmanagedFunctionPointer(CallingConvention.StdCall)]
internal delegate int QueryInterfaceDelegate(IntPtr self, ref Guid riid, out IntPtr ppvObject);

[UnmanagedFunctionPointer(CallingConvention.StdCall)]
internal delegate uint ReleaseDelegate(IntPtr self);

[UnmanagedFunctionPointer(CallingConvention.StdCall)]
internal delegate int GetParentDelegate(IntPtr self, ref Guid riid, out IntPtr parent);

[UnmanagedFunctionPointer(CallingConvention.StdCall)]
internal delegate int GetAdapterDelegate(IntPtr dxgiDevice, out IntPtr adapter);

[UnmanagedFunctionPointer(CallingConvention.StdCall)]
internal delegate int EnumAdapters1Delegate(IntPtr factory, uint adapterIndex, out IntPtr adapter);

[UnmanagedFunctionPointer(CallingConvention.StdCall)]
internal delegate int GetDesc1Delegate(IntPtr adapter, out DXGI_ADAPTER_DESC1 desc);

[UnmanagedFunctionPointer(CallingConvention.StdCall)]
internal delegate int PresentDelegate(IntPtr swapChain, uint syncInterval, uint flags);

[UnmanagedFunctionPointer(CallingConvention.StdCall)]
internal delegate int GetBufferDelegate(IntPtr swapChain, uint buffer, ref Guid riid, out IntPtr ppSurface);

[UnmanagedFunctionPointer(CallingConvention.StdCall)]
internal delegate int ResizeBuffersDelegate(IntPtr swapChain, uint bufferCount, uint width, uint height, DXGI_FORMAT format, uint flags);

[UnmanagedFunctionPointer(CallingConvention.StdCall)]
internal delegate int CreateSwapChainForCompositionDelegate(IntPtr factory, IntPtr device, ref DXGI_SWAP_CHAIN_DESC1 desc, IntPtr restrictToOutput, out IntPtr swapChain);

[UnmanagedFunctionPointer(CallingConvention.StdCall)]
internal delegate int OpenSharedResource1Delegate(IntPtr device1, IntPtr hResource, ref Guid riid, out IntPtr ppResource);

// ============================================================================
// ID3D11Device — Additional method delegates
// ============================================================================

[UnmanagedFunctionPointer(CallingConvention.StdCall)]
internal delegate int CreateRenderTargetViewDelegate(
    IntPtr device,
    IntPtr pResource,       // ID3D11Resource*
    IntPtr pDesc,           // D3D11_RENDER_TARGET_VIEW_DESC* or IntPtr.Zero
    out IntPtr ppRTView);   // ID3D11RenderTargetView**

[UnmanagedFunctionPointer(CallingConvention.StdCall)]
internal delegate int CreateTexture2DDelegate(
    IntPtr device,
    IntPtr pDesc,           // D3D11_TEXTURE2D_DESC*
    IntPtr pInitialData,    // D3D11_SUBRESOURCE_DATA* or IntPtr.Zero
    out IntPtr ppTexture2D);// ID3D11Texture2D**

// ============================================================================
// ID3D11DeviceContext — Method delegates
// ============================================================================

[UnmanagedFunctionPointer(CallingConvention.StdCall)]
internal delegate int ClearRenderTargetViewDelegate(
    IntPtr context,
    IntPtr pRenderTargetView,  // ID3D11RenderTargetView*
    ref float pColorRGBA);     // float[4] RGBA color

[UnmanagedFunctionPointer(CallingConvention.StdCall)]
internal delegate void CopyResourceDelegate(
    IntPtr context,
    IntPtr pDstResource,  // ID3D11Resource*
    IntPtr pSrcResource); // ID3D11Resource*

// ============================================================================
// ISwapChainPanelNative — Method delegates
// ============================================================================

[UnmanagedFunctionPointer(CallingConvention.StdCall)]
internal delegate int SetSwapChainDelegate(
    IntPtr native,          // ISwapChainPanelNative*
    IntPtr swapChain);      // IDXGISwapChain*

// ============================================================================
// Vtbl Helper — Extract function pointers from COM vtable
// ============================================================================
internal static unsafe class Vtbl
{
    internal static T GetMethod<T>(IntPtr comPtr, int methodIndex) where T : Delegate
    {
        IntPtr* vtbl = *(IntPtr**)comPtr;
        return Marshal.GetDelegateForFunctionPointer<T>(vtbl[methodIndex]);
    }

    internal static uint Release(IntPtr comPtr)
    {
        var del = GetMethod<ReleaseDelegate>(comPtr, COMVtbl.Release);
        return del(comPtr);
    }

    internal static int QueryInterface(IntPtr comPtr, Guid riid, out IntPtr ppv)
    {
        var del = GetMethod<QueryInterfaceDelegate>(comPtr, COMVtbl.QueryInterface);
        return del(comPtr, ref riid, out ppv);
    }
}

// ============================================================================
// COM VTable Offset Constants
// Based on Windows 10 SDK 10.0.19041.0 (matching project TFM)
//
// VTable layout inheritance:
//   IUnknown:              [0] QI  [1] AddRef  [2] Release
//   IDXGIObject:           [3-6]   (4 methods: SetPrivateData, SetPrivateDataInterface,
//                                          GetPrivateData, GetParent)
//   IDXGIDevice:           inherits IDXGIObject, adds [7] GetAdapter ...
//   IDXGIFactory:          inherits IDXGIObject, adds [7-11]
//   IDXGIFactory1:         inherits IDXGIFactory, adds [12-13]
//   IDXGIFactory2:         inherits IDXGIFactory1, adds [14-24]
//   IDXGIAdapter:          inherits IDXGIObject, adds [7-9]
//   IDXGIAdapter1:         inherits IDXGIAdapter, adds [10]
//   IDXGISwapChain:        inherits IDXGIDeviceSubObject (IDXGIObject + 1),
//                          adds [8-17]
//   ID3D11Device:          IUnknown + 39 methods [3-41]
//   ID3D11Device1:         inherits ID3D11Device, adds 11 methods [42-52]
// ============================================================================
internal static class COMVtbl
{
    // --- IUnknown (all COM interfaces) ---
    public const int QueryInterface = 0;
    public const int AddRef         = 1;
    public const int Release        = 2;

    // --- IDXGIObject ---
    public const int SetPrivateData          = 3;
    public const int SetPrivateDataInterface = 4;
    public const int GetPrivateData          = 5;
    public const int GetParent               = 6;

    // --- IDXGIDevice (inherits IDXGIObject) ---
    public const int IDXGIDevice_GetAdapter   = 7;

    // --- IDXGIFactory (inherits IDXGIObject) ---
    public const int IDXGIFactory_EnumAdapters = 7;
    // ...

    // --- IDXGIFactory1 (inherits IDXGIFactory) ---
    public const int IDXGIFactory1_EnumAdapters1 = 12;

    // --- IDXGIFactory2 (inherits IDXGIFactory1) ---
    // IDXGIFactory2 adds 11 methods at indices 14-24
    // CreateSwapChainForComposition is the last of these
    public const int IDXGIFactory2_CreateSwapChainForComposition = 24;

    // --- IDXGIAdapter (inherits IDXGIObject) ---
    // IDXGIAdapter adds 3 methods: EnumOutputs [7], GetDesc [8], CheckInterfaceSupport [9]

    // --- IDXGIAdapter1 (inherits IDXGIAdapter) ---
    public const int IDXGIAdapter1_GetDesc1 = 10;

    // --- IDXGISwapChain1 (inherits IDXGISwapChain → IDXGIDeviceSubObject → IDXGIObject) ---
    // IDXGIDeviceSubObject adds GetDevice [7] to IDXGIObject
    // IDXGISwapChain adds [8] Present, [9] GetBuffer, ... [13] ResizeBuffers, ... [17] GetLastPresentCount
    public const int IDXGISwapChain_Present       = 8;
    public const int IDXGISwapChain_GetBuffer     = 9;
    public const int IDXGISwapChain_ResizeBuffers = 13;

    // --- ID3D11Device (inherits IUnknown) ---
    // ID3D11Device adds 39 methods [3-41]
    // CreateRenderTargetView is method 6 (0-indexed) of ID3D11Device = index 9
    public const int ID3D11Device_CreateRenderTargetView = 9;
    public const int ID3D11Device_CreateTexture2D        = 5;

    // --- ID3D11Device1 (inherits ID3D11Device → IUnknown) ---
    // ID3D11Device1 adds 11 methods [42-52]; OpenSharedResource1 is last
    public const int ID3D11Device1_OpenSharedResource1 = 52;

    // --- ID3D11DeviceContext (inherits ID3D11DeviceChild → IUnknown) ---
    // ID3D11DeviceChild adds 3 methods [3-5]
    // ID3D11DeviceContext adds 39 methods starting at [6]
    // ClearRenderTargetView is method 38 (0-indexed) = index 44
    // CopyResource is method 35 (0-indexed) = index 41
    public const int ID3D11DeviceContext_ClearRenderTargetView = 44;
    public const int ID3D11DeviceContext_CopyResource          = 41;

    // --- ISwapChainPanelNative (IUnknown + SetSwapChain) ---
    public const int ISwapChainPanelNative_SetSwapChain = 3;
}

// ============================================================================
// HRESULT helper
// ============================================================================
internal static class HResult
{
    internal static void ThrowOnFailure(int hr)
    {
        if (hr < 0)
            Marshal.ThrowExceptionForHR(hr);
    }

    internal static bool Succeeded(int hr) => hr >= 0;
    internal static bool Failed(int hr) => hr < 0;
}

// ============================================================================
// D3D11Device — Direct3D 11 device wrapper
// ============================================================================
/// <summary>
/// Wraps an ID3D11Device with adapter LUID tracking for Vulkan interop.
/// The device is created with D3D11_CREATE_DEVICE_BGRA_SUPPORT for SwapChainPanel compatibility.
/// </summary>
internal unsafe class D3D11Device : IDisposable
{
    private bool _disposed;

    /// <summary>ID3D11Device*</summary>
    public IntPtr NativeDevice { get; }

    /// <summary>ID3D11DeviceContext*</summary>
    public IntPtr ImmediateContext { get; }

    /// <summary>
    /// Adapter LUID (Locally Unique Identifier) used to match this D3D11 device
    /// with the Vulkan physical device for cross-API memory sharing.
    /// </summary>
    public long Luid { get; }

    private D3D11Device(IntPtr device, IntPtr context, long luid)
    {
        NativeDevice = device;
        ImmediateContext = context;
        Luid = luid;
    }

    /// <summary>
    /// Creates a D3D11 device, optionally matching a specific GPU adapter by LUID.
    /// </summary>
    /// <param name="preferredLuid">
    /// If non-null, enumerates all DXGI adapters to find one matching this LUID.
    /// If null, creates the device on the default adapter.
    /// </param>
    /// <param name="debug">Enable D3D11 debug layer.</param>
    /// <param name="forceWarp">Use WARP software rasterizer.</param>
    /// <returns>A new D3D11Device, or null if creation fails.</returns>
    public static D3D11Device? Create(
        long?  preferredLuid = null,
        bool   debug         = false,
        bool   forceWarp     = false)
    {
        // Build creation flags
        uint flags = (uint)D3D11_CREATE_DEVICE_FLAG.BgraSupport;
        if (debug)
            flags |= (uint)D3D11_CREATE_DEVICE_FLAG.Debug;

        IntPtr adapter = IntPtr.Zero;
        D3D_DRIVER_TYPE driverType;

        if (preferredLuid.HasValue)
        {
            adapter = FindAdapterByLuid(preferredLuid.Value);
            if (adapter == IntPtr.Zero)
                return null; // matching adapter not found
            driverType = D3D_DRIVER_TYPE.Unknown;
        }
        else
        {
            adapter = IntPtr.Zero;
            driverType = forceWarp ? D3D_DRIVER_TYPE.Warp : D3D_DRIVER_TYPE.Hardware;
        }

        try
        {
            // Request feature levels: try 11.1 first, fall back to 11.0
            D3D_FEATURE_LEVEL[] levels =
            {
                D3D_FEATURE_LEVEL._11_1,
                D3D_FEATURE_LEVEL._11_0,
            };

            int hr = NativeMethodsD3D.D3D11CreateDevice(
                adapter,
                driverType,
                IntPtr.Zero,
                flags,
                ref levels[0],
                (uint)levels.Length,
                D3D11Constants.SDKVersion,
                out IntPtr device,
                out D3D_FEATURE_LEVEL selectedLevel,
                out IntPtr context);

            if (HResult.Failed(hr))
                return null;

            // Get the adapter LUID for this device
            long luid = QueryDeviceLuid(device);

            return new D3D11Device(device, context, luid);
        }
        finally
        {
            if (adapter != IntPtr.Zero)
                Vtbl.Release(adapter);
        }
    }

    // ================================================================
    // Adapter enumeration
    // ================================================================

    /// <summary>
    /// Enumerates all DXGI adapters and returns the first whose LUID matches.
    /// </summary>
    private static IntPtr FindAdapterByLuid(long targetLuid)
    {
        Guid factoryGuid = DXGIGuid.IDXGIFactory1;
        int hr = NativeMethodsD3D.CreateDXGIFactory1(
            ref factoryGuid, out IntPtr factory);

        if (HResult.Failed(hr))
            return IntPtr.Zero;

        try
        {
            var enumAdapters1 = Vtbl.GetMethod<EnumAdapters1Delegate>(
                factory, COMVtbl.IDXGIFactory1_EnumAdapters1);

            for (uint i = 0; ; i++)
            {
                hr = enumAdapters1(factory, i, out IntPtr adapter);
                if (HResult.Failed(hr))
                    break; // DXGI_ERROR_NOT_FOUND

                var getDesc1 = Vtbl.GetMethod<GetDesc1Delegate>(
                    adapter, COMVtbl.IDXGIAdapter1_GetDesc1);

                hr = getDesc1(adapter, out DXGI_ADAPTER_DESC1 desc);
                if (HResult.Succeeded(hr) && desc.AdapterLuid.ToInt64() == targetLuid)
                    return adapter; // caller must release

                Vtbl.Release(adapter);
            }
        }
        finally
        {
            Vtbl.Release(factory);
        }

        return IntPtr.Zero;
    }

    /// <summary>
    /// Retrieves the LUID of the adapter associated with a D3D11 device.
    /// </summary>
    private static long QueryDeviceLuid(IntPtr device)
    {
        // ID3D11Device → IDXGIDevice → IDXGIAdapter → IDXGIAdapter1 → GetDesc1
        int hr = Vtbl.QueryInterface(device, DXGIGuid.IDXGIDevice, out IntPtr dxgiDevice);
        if (HResult.Failed(hr))
            return 0;

        try
        {
            var getAdapter = Vtbl.GetMethod<GetAdapterDelegate>(
                dxgiDevice, COMVtbl.IDXGIDevice_GetAdapter);

            hr = getAdapter(dxgiDevice, out IntPtr adapter);
            if (HResult.Failed(hr))
                return 0;

            try
            {
                hr = Vtbl.QueryInterface(adapter, DXGIGuid.IDXGIAdapter1, out IntPtr adapter1);
                if (HResult.Failed(hr))
                    return 0;

                try
                {
                    var getDesc1 = Vtbl.GetMethod<GetDesc1Delegate>(
                        adapter1, COMVtbl.IDXGIAdapter1_GetDesc1);

                    hr = getDesc1(adapter1, out DXGI_ADAPTER_DESC1 desc);
                    if (HResult.Succeeded(hr))
                        return desc.AdapterLuid.ToInt64();
                }
                finally
                {
                    Vtbl.Release(adapter1);
                }
            }
            finally
            {
                Vtbl.Release(adapter);
            }
        }
        finally
        {
            Vtbl.Release(dxgiDevice);
        }

        return 0;
    }

    // ================================================================
    // IDisposable
    // ================================================================

    public void Dispose()
    {
        if (_disposed) return;
        _disposed = true;

        if (ImmediateContext != IntPtr.Zero)
        {
            Vtbl.Release(ImmediateContext);
        }

        if (NativeDevice != IntPtr.Zero)
        {
            Vtbl.Release(NativeDevice);
        }
    }
}

// ============================================================================
// SharedTextureManager — Imports Vulkan-exported Win32 handles as D3D11 textures
// ============================================================================
/// <summary>
/// Manages the lifecycle of D3D11 shared textures imported from Vulkan via
/// ID3D11Device1::OpenSharedResource1.
///
/// The imported texture shares GPU memory with a Vulkan VkImage, enabling
/// cross-API zero-copy rendering.
/// </summary>
internal unsafe class SharedTextureManager : IDisposable
{
    private readonly IntPtr _d3d11Device1Ptr;  // ID3D11Device1* (via QI)
    private readonly Dictionary<IntPtr, IntPtr> _sharedTextures = new();
    private bool _disposed;

    public SharedTextureManager(IntPtr d3d11Device)
    {
        int hr = Vtbl.QueryInterface(
            d3d11Device, D3D11Guid.ID3D11Device1, out _d3d11Device1Ptr);

        if (HResult.Failed(hr))
            throw new InvalidOperationException(
                $"Failed to query ID3D11Device1. HRESULT: {hr}");
    }

    /// <summary>
    /// Imports a Win32 shared handle (exported from Vulkan via
    /// vkGetMemoryWin32HandleKHR) as an ID3D11Texture2D.
    /// </summary>
    /// <param name="win32Handle">NT handle exported from Vulkan.</param>
    /// <param name="width">Texture width in pixels.</param>
    /// <param name="height">Texture height in pixels.</param>
    /// <param name="format">DXGI format matching the Vulkan image format.</param>
    /// <returns>ID3D11Texture2D pointer, or IntPtr.Zero on failure.</returns>
    public IntPtr ImportSharedTexture(
        IntPtr           win32Handle,
        int              width,
        int              height,
        DXGI_FORMAT      format = DXGI_FORMAT.B8G8R8A8_UNorm)
    {
        if (win32Handle == IntPtr.Zero)
            return IntPtr.Zero;

        var openSharedRes1 = Vtbl.GetMethod<OpenSharedResource1Delegate>(
            _d3d11Device1Ptr, COMVtbl.ID3D11Device1_OpenSharedResource1);

        Guid iid = D3D11Guid.ID3D11Texture2D;
        int hr = openSharedRes1(_d3d11Device1Ptr, win32Handle, ref iid, out IntPtr texture);

        if (HResult.Failed(hr))
            return IntPtr.Zero;

        _sharedTextures[win32Handle] = texture;
        return texture;
    }

    /// <summary>
    /// Releases a previously imported shared texture.
    /// </summary>
    public void ReleaseSharedTexture(IntPtr win32Handle)
    {
        if (_sharedTextures.TryGetValue(win32Handle, out IntPtr texture))
        {
            Vtbl.Release(texture);
            _sharedTextures.Remove(win32Handle);
        }
    }

    public void Dispose()
    {
        if (_disposed) return;
        _disposed = true;

        foreach (var kvp in _sharedTextures)
            Vtbl.Release(kvp.Value);
        _sharedTextures.Clear();

        if (_d3d11Device1Ptr != IntPtr.Zero)
            Vtbl.Release(_d3d11Device1Ptr);
    }
}

// ============================================================================
// SwapChainManager — DXGI composition swap chain for SwapChainPanel
// ============================================================================
/// <summary>
/// Creates and manages an IDXGISwapChain1 suitable for use with a WinUI3
/// SwapChainPanel via the ISwapChainPanelNative interface.
///
/// Uses CreateSwapChainForComposition for overlay/composition scenarios.
/// </summary>
internal unsafe class SwapChainManager : IDisposable
{
    private bool _disposed;

    /// <summary>IDXGISwapChain1*</summary>
    public IntPtr SwapChain { get; private set; }

    /// <summary>
    /// Creates a DXGI composition swap chain backed by the given D3D11 device.
    /// </summary>
    /// <param name="d3d11Device">ID3D11Device*</param>
    /// <param name="width">Initial back-buffer width.</param>
    /// <param name="height">Initial back-buffer height.</param>
    /// <param name="format">Back-buffer format (default B8G8R8A8_UNORM).</param>
    public void CreateCompositionSwapChain(
        IntPtr      d3d11Device,
        int         width,
        int         height,
        DXGI_FORMAT format = DXGI_FORMAT.B8G8R8A8_UNorm)
    {
        if (SwapChain != IntPtr.Zero)
            return; // already created — destroy first via Dispose if needed

        // ============================================================
        // 1. ID3D11Device → IDXGIDevice
        // ============================================================
        int hr = Vtbl.QueryInterface(
            d3d11Device, DXGIGuid.IDXGIDevice, out IntPtr dxgiDevice);
        HResult.ThrowOnFailure(hr);

        try
        {
            // ============================================================
            // 2. IDXGIDevice → IDXGIAdapter
            // ============================================================
            var getAdapter = Vtbl.GetMethod<GetAdapterDelegate>(
                dxgiDevice, COMVtbl.IDXGIDevice_GetAdapter);

            hr = getAdapter(dxgiDevice, out IntPtr adapter);
            HResult.ThrowOnFailure(hr);

            try
            {
                // ============================================================
                // 3. IDXGIAdapter → IDXGIFactory2 (via GetParent)
                // ============================================================
                var getParent = Vtbl.GetMethod<GetParentDelegate>(
                    adapter, COMVtbl.GetParent);

                Guid factory2Guid = DXGIGuid.IDXGIFactory2;
                hr = getParent(adapter, ref factory2Guid, out IntPtr factory);
                HResult.ThrowOnFailure(hr);

                try
                {
                    // ============================================================
                    // 4. Create the composition swap chain
                    // ============================================================
                    var createSwapChain = Vtbl.GetMethod<CreateSwapChainForCompositionDelegate>(
                        factory, COMVtbl.IDXGIFactory2_CreateSwapChainForComposition);

                    var desc = new DXGI_SWAP_CHAIN_DESC1
                    {
                        Width       = (uint)width,
                        Height      = (uint)height,
                        Format      = format,
                        Stereo      = 0,
                        SampleDesc  = new DXGI_SAMPLE_DESC { Count = 1, Quality = 0 },
                        BufferUsage = DXGI_USAGE.RenderTargetOutput,
                        BufferCount = 2, // double buffering
                        Scaling     = DXGI_SCALING.Stretch,
                        SwapEffect  = DXGI_SWAP_EFFECT.FlipDiscard,
                        AlphaMode   = DXGI_ALPHA_MODE.Ignore,
                        Flags       = (uint)DXGI_SWAP_CHAIN_FLAG.None,
                    };

                    hr = createSwapChain(factory, d3d11Device, ref desc, IntPtr.Zero, out IntPtr swapChain);
                    HResult.ThrowOnFailure(hr);

                    SwapChain = swapChain;
                }
                finally
                {
                    Vtbl.Release(factory);
                }
            }
            finally
            {
                Vtbl.Release(adapter);
            }
        }
        finally
        {
            Vtbl.Release(dxgiDevice);
        }
    }

    /// <summary>
    /// Resizes the swap chain buffers (call when the SwapChainPanel is resized).
    /// </summary>
    public void ResizeBuffers(int width, int height)
    {
        if (SwapChain == IntPtr.Zero)
            return;

        var resize = Vtbl.GetMethod<ResizeBuffersDelegate>(
            SwapChain, COMVtbl.IDXGISwapChain_ResizeBuffers);

        // Preserve buffer count & format (pass 0 and Unknown to keep current)
        int hr = resize(SwapChain, 0, (uint)width, (uint)height, DXGI_FORMAT.Unknown, 0);
        HResult.ThrowOnFailure(hr);
    }

    /// <summary>
    /// Presents the rendered frame.
    /// </summary>
    /// <param name="vsync">If true, wait for vertical sync (Present(1,0)).</param>
    public void Present(bool vsync = false)
    {
        if (SwapChain == IntPtr.Zero)
            return;

        var present = Vtbl.GetMethod<PresentDelegate>(
            SwapChain, COMVtbl.IDXGISwapChain_Present);

        int hr = present(SwapChain, vsync ? 1u : 0u, 0);
        HResult.ThrowOnFailure(hr);
    }

    /// <summary>
    /// Returns the back buffer (ID3D11Texture2D*) for the swap chain.
    /// Caller must Release the returned pointer when done.
    /// </summary>
    public IntPtr GetBackBuffer()
    {
        if (SwapChain == IntPtr.Zero)
            return IntPtr.Zero;

        var getBuffer = Vtbl.GetMethod<GetBufferDelegate>(
            SwapChain, COMVtbl.IDXGISwapChain_GetBuffer);

        Guid iid = D3D11Guid.ID3D11Texture2D;
        int hr = getBuffer(SwapChain, 0, ref iid, out IntPtr backBuffer);
        HResult.ThrowOnFailure(hr);

        return backBuffer; // caller must Release
    }

    public void Dispose()
    {
        if (_disposed) return;
        _disposed = true;

        if (SwapChain != IntPtr.Zero)
        {
            Vtbl.Release(SwapChain);
            SwapChain = IntPtr.Zero;
        }
    }
}

// ============================================================================
// ISwapChainPanelNative — WinUI3 COM interface for SwapChainPanel binding
// ============================================================================
/// <summary>
/// COM interface for associating a DXGI swap chain with a WinUI3 SwapChainPanel.
/// GUID: 3E3F1D7D-E21F-43E1-BC1C-FAD9306572D7
/// </summary>
[ComImport]
[Guid("3E3F1D7D-E21F-43E1-BC1C-FAD9306572D7")]
[InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
internal interface ISwapChainPanelNative
{
    /// <summary>
    /// Sets the DXGI swap chain for the SwapChainPanel.
    /// </summary>
    /// <param name="swapChain">IDXGISwapChain* pointer.</param>
    void SetSwapChain(IntPtr swapChain);
}

// ============================================================================
// D3D11Extensions — Format conversion helpers
// ============================================================================
internal static class D3D11Extensions
{
    /// <summary>
    /// Converts a Vulkan VkFormat to the equivalent DXGI_FORMAT.
    /// Only common formats used for interop rendering are handled.
    /// </summary>
    internal static DXGI_FORMAT VkFormatToDxgi(int vkFormat)
    {
        // VK_FORMAT enum values (only the ones we care about)
        const int VK_FORMAT_B8G8R8A8_UNORM    = 44;
        const int VK_FORMAT_B8G8R8A8_SRGB     = 45;
        const int VK_FORMAT_R8G8B8A8_UNORM    = 37;
        const int VK_FORMAT_R8G8B8A8_SRGB     = 43;
        const int VK_FORMAT_R16G16B16A16_SFLOAT = 97;
        const int VK_FORMAT_R32G32B32A32_SFLOAT = 109;
        const int VK_FORMAT_D32_SFLOAT         = 126;
        const int VK_FORMAT_D24_UNORM_S8_UINT  = 129;

        return vkFormat switch
        {
            VK_FORMAT_B8G8R8A8_UNORM        => DXGI_FORMAT.B8G8R8A8_UNorm,
            VK_FORMAT_B8G8R8A8_SRGB         => DXGI_FORMAT.B8G8R8A8_UNorm_SRgb,
            VK_FORMAT_R8G8B8A8_UNORM        => DXGI_FORMAT.R8G8B8A8_UNorm,
            VK_FORMAT_R8G8B8A8_SRGB         => DXGI_FORMAT.R8G8B8A8_UNorm_SRgb,
            VK_FORMAT_R16G16B16A16_SFLOAT   => DXGI_FORMAT.R16G16B16A16_Float,
            VK_FORMAT_R32G32B32A32_SFLOAT   => DXGI_FORMAT.R32G32B32A32_Float,
            VK_FORMAT_D32_SFLOAT             => DXGI_FORMAT.D32_Float,
            VK_FORMAT_D24_UNORM_S8_UINT      => DXGI_FORMAT.D24_UNorm_S8_UInt,
            _                                => DXGI_FORMAT.Unknown,
        };
    }
}
