using System.Runtime.InteropServices;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;

namespace NeoEditor.Core.Interop;

/// Manages the D3D11 SwapChainPanel integration for the 3D viewport.
///
/// Responsibilities:
///   - Create D3D11 device and DXGI composition swap chain
///   - Bind swap chain to SwapChainPanel via ISwapChainPanelNative
///   - Frame pump via CompositionTarget.Rendering (clear + Present)
///   - Resize handling with DPI-aware physical pixel calculation
///   - Receive shared texture handles from Vulkan engine (for future T19 integration)
///
/// MVP mode: Each frame clears the back buffer to a dark background color
/// and presents. Full Vulkan→D3D11 texture copy will be wired in T19.
internal sealed class ViewportSurface : IDisposable
{
    // Clear color: dark blue-gray
    private static readonly float[] s_clearColor = [0.07f, 0.07f, 0.12f, 1.0f];

    private readonly SwapChainPanel _panel;

    // D3D11 resources
    private D3D11Device? _d3d11Device;
    private SwapChainManager? _swapChainManager;
    private SharedTextureManager? _sharedTextureManager;

    // Cached swap chain resources (released/created on resize)
    private IntPtr _backBuffer; // ID3D11Texture2D*
    private IntPtr _rtv;       // ID3D11RenderTargetView*

    // Shared texture import (thread-safe via _handleLock)
    private IntPtr _importedTexture; // ID3D11Texture2D* from Vulkan
    private IntPtr _pendingSharedHandle;
    private readonly object _handleLock = new();

    // Current pixel size (DPI-scaled)
    private int _width;
    private int _height;

    // Engine queue for draining on UI thread each frame
    private EngineToUIQueue? _engineQueue;

    private bool _subscribed;
    private bool _disposed;

    public ViewportSurface(SwapChainPanel panel)
    {
        ArgumentNullException.ThrowIfNull(panel);
        _panel = panel;
    }

    // ================================================================
    // Initialization
    // ================================================================

    /// Creates D3D11 device, composition swap chain, binds to SwapChainPanel,
    /// and starts the frame pump.
    public void Initialize(long? vulkanLuid = null)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        if (_subscribed)
            return;

        _d3d11Device = D3D11Device.Create(vulkanLuid, debug: false);
        if (_d3d11Device == null)
            throw new InvalidOperationException(
                "Failed to create D3D11 device for viewport. " +
                "Ensure a compatible GPU is available.");

        _sharedTextureManager = new SharedTextureManager(_d3d11Device.NativeDevice);

        _swapChainManager = new SwapChainManager();

        UpdatePixelSize();
        _swapChainManager.CreateCompositionSwapChain(
            _d3d11Device.NativeDevice,
            Math.Max(1, _width),
            Math.Max(1, _height));

        BindToPanel();
        CacheBackBuffer();
        CreateRtv();

        CompositionTarget.Rendering += OnRendering;
        _subscribed = true;
    }

    /// Optionally connects the engine-to-UI message queue so that engine
    /// commands (including shared texture handle updates) are drained on the
    /// UI thread each frame during the frame pump.
    public void SetEngineQueue(EngineToUIQueue queue)
    {
        _engineQueue = queue;
    }

    /// Called from UI thread (typically during EngineToUIQueue drain) when
    /// the Vulkan engine exports a new shared texture handle for display.
    public void SetSharedTextureHandle(IntPtr win32Handle)
    {
        lock (_handleLock)
        {
            _pendingSharedHandle = win32Handle;
        }
    }

    // ================================================================
    // Resize
    // ================================================================

    /// Recomputes physical pixel size from the panel's actual dimensions
    /// and DPI scale, then resizes the swap chain and recreates the RTV.
    public void Resize()
    {
        if (_swapChainManager == null || _disposed)
            return;

        UpdatePixelSize();

        if (_width < 1 || _height < 1)
            return;

        // Release cached back buffer and RTV before resize
        ReleaseBackBuffer();
        ReleaseRtv();

        _swapChainManager.ResizeBuffers(_width, _height);

        CacheBackBuffer();
        CreateRtv();
    }

    // ================================================================
    // Pause / Resume rendering (used by WindowManager for minimize/restore)
    // ================================================================

    /// Pauses the frame pump by unsubscribing from CompositionTarget.Rendering.
    /// Called when the window is minimized (to save CPU/GPU resources).
    public void PauseRendering()
    {
        if (_subscribed)
        {
            CompositionTarget.Rendering -= OnRendering;
            _subscribed = false;
        }
    }

    /// Resumes the frame pump by re-subscribing to CompositionTarget.Rendering.
    /// Called when the window is restored from minimized.
    public void ResumeRendering()
    {
        if (!_subscribed && !_disposed)
        {
            CompositionTarget.Rendering += OnRendering;
            _subscribed = true;
        }
    }

    // ================================================================
    // Frame pump
    // ================================================================

    private void OnRendering(object? sender, object? e)
    {
        if (_disposed || _swapChainManager == null)
            return;

        // Drain engine queue on UI thread
        _engineQueue?.Drain(action => action());

        // Import pending shared texture handle (if any)
        ImportPendingHandle();

        // Clear back buffer
        if (_rtv != IntPtr.Zero)
        {
            var clearRtv = Vtbl.GetMethod<ClearRenderTargetViewDelegate>(
                _d3d11Device!.ImmediateContext,
                COMVtbl.ID3D11DeviceContext_ClearRenderTargetView);

            clearRtv(_d3d11Device.ImmediateContext, _rtv, ref s_clearColor[0]);
        }

        // TODO(T19): Copy imported shared texture to back buffer
        // if (_importedTexture != IntPtr.Zero)
        // {
        //     var copyResource = Vtbl.GetMethod<CopyResourceDelegate>(
        //         _d3d11Device!.ImmediateContext,
        //         COMVtbl.ID3D11DeviceContext_CopyResource);
        //     copyResource(_d3d11Device.ImmediateContext, _backBuffer, _importedTexture);
        // }

        _swapChainManager.Present(vsync: true);
    }

    // ================================================================
    // Handle import
    // ================================================================

    private void ImportPendingHandle()
    {
        IntPtr handle;
        lock (_handleLock)
        {
            if (_pendingSharedHandle == IntPtr.Zero)
                return;
            handle = _pendingSharedHandle;
            _pendingSharedHandle = IntPtr.Zero;
        }

        // Release previous imported texture
        if (_importedTexture != IntPtr.Zero)
        {
            Vtbl.Release(_importedTexture);
            _importedTexture = IntPtr.Zero;
        }

        // Import the new shared texture
        _importedTexture = _sharedTextureManager!.ImportSharedTexture(
            handle, _width, _height);
    }

    // ================================================================
    // SwapChainPanel binding
    // ================================================================

    private void BindToPanel()
    {
        Guid iid = DXGIGuid.ISwapChainPanelNative;

        IntPtr pUnknown = Marshal.GetIUnknownForObject(_panel);
        try
        {
            int hr = Marshal.QueryInterface(pUnknown, ref iid, out IntPtr pNative);
            HResult.ThrowOnFailure(hr);
            try
            {
                var setSwapChain = Vtbl.GetMethod<SetSwapChainDelegate>(
                    pNative, COMVtbl.ISwapChainPanelNative_SetSwapChain);

                hr = setSwapChain(pNative, _swapChainManager!.SwapChain);
                HResult.ThrowOnFailure(hr);
            }
            finally
            {
                Marshal.Release(pNative);
            }
        }
        finally
        {
            Marshal.Release(pUnknown);
        }
    }

    // ================================================================
    // Back buffer & RTV management
    // ================================================================

    private void CacheBackBuffer()
    {
        _backBuffer = _swapChainManager!.GetBackBuffer();
    }

    private void ReleaseBackBuffer()
    {
        if (_backBuffer != IntPtr.Zero)
        {
            Vtbl.Release(_backBuffer);
            _backBuffer = IntPtr.Zero;
        }
    }

    private void CreateRtv()
    {
        if (_backBuffer == IntPtr.Zero || _d3d11Device == null)
            return;

        var createRtv = Vtbl.GetMethod<CreateRenderTargetViewDelegate>(
            _d3d11Device.NativeDevice,
            COMVtbl.ID3D11Device_CreateRenderTargetView);

        int hr = createRtv(
            _d3d11Device.NativeDevice,
            _backBuffer,
            IntPtr.Zero, // default desc (entire resource, matching format)
            out _rtv);

        if (HResult.Failed(hr))
        {
            _rtv = IntPtr.Zero;
        }
    }

    private void ReleaseRtv()
    {
        if (_rtv != IntPtr.Zero)
        {
            Vtbl.Release(_rtv);
            _rtv = IntPtr.Zero;
        }
    }

    // ================================================================
    // Pixel size calculation
    // ================================================================

    private void UpdatePixelSize()
    {
        int w = (int)(_panel.ActualWidth * _panel.CompositionScaleX);
        int h = (int)(_panel.ActualHeight * _panel.CompositionScaleY);

        _width = Math.Max(1, w);
        _height = Math.Max(1, h);
    }

    // ================================================================
    // IDisposable
    // ================================================================

    public void Dispose()
    {
        if (_disposed)
            return;
        _disposed = true;

        if (_subscribed)
        {
            CompositionTarget.Rendering -= OnRendering;
            _subscribed = false;
        }

        ReleaseRtv();
        ReleaseBackBuffer();

        if (_importedTexture != IntPtr.Zero)
        {
            Vtbl.Release(_importedTexture);
            _importedTexture = IntPtr.Zero;
        }

        _sharedTextureManager?.Dispose();
        _swapChainManager?.Dispose();
        _d3d11Device?.Dispose();
    }
}
