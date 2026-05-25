using Windows.Graphics.Display;
using Windows.UI.Core;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace NeoEditor.Core;

/// 窗口管理器 (T15): 集中处理 WinUI3 窗口生命周期事件，链式触发视口纹理尺寸更新。
///
/// 职责:
///   - 窗口尺寸变化 → ViewportSurface.Resize()
///   - DPI 变化 → 重新计算物理像素尺寸
///   - 最小化/恢复 → 暂停/恢复渲染帧泵
///   - 窗口关闭 → 先停止引擎线程，再释放资源
internal sealed class WindowManager : IDisposable
{
    private readonly Window _window;
    private readonly SwapChainPanel _viewport;
    private readonly ViewportSurface _viewportSurface;
    private readonly EngineThread? _engineThread;
    private readonly DisplayInformation _displayInfo;
    private readonly CoreWindow _coreWindow;
    private bool _disposed;

窗口最小化时触发，UI 层应暂停帧泵。</summary>
    public event Action? RenderingPaused;

窗口恢复时触发，UI 层应恢复帧泵。</summary>
    public event Action? RenderingResumed;

    public WindowManager(
        Window window,
        SwapChainPanel viewport,
        ViewportSurface viewportSurface,
        EngineThread? engineThread)
    {
        ArgumentNullException.ThrowIfNull(window);
        ArgumentNullException.ThrowIfNull(viewport);
        ArgumentNullException.ThrowIfNull(viewportSurface);

        _window = window;
        _viewport = viewport;
        _viewportSurface = viewportSurface;
        _engineThread = engineThread;

        // 缓存 DPI 和 CoreWindow 引用以便在 Dispose 时安全取消订阅
        _displayInfo = DisplayInformation.GetForCurrentView();
        _coreWindow = CoreWindow.GetForCurrentThread();
    }

订阅所有窗口生命周期事件。在 ViewportSurface 初始化完成后调用。</summary>
    public void Start()
    {
        ObjectDisposedException.ThrowIf(_disposed, this);

        // 窗口尺寸变化 (整体窗口级别)
        _window.SizeChanged += OnWindowSizeChanged;

        // DPI 变化
        _displayInfo.DpiChanged += OnDpiChanged;

        // 可见性变化 (最小化/恢复)
        _coreWindow.VisibilityChanged += OnVisibilityChanged;

        // 窗口关闭
        _window.Closed += OnWindowClosing;
    }

取消订阅所有事件。在 Dispose 前调用以确保有序拆卸。</summary>
    public void Stop()
    {
        _window.SizeChanged -= OnWindowSizeChanged;
        _window.Closed -= OnWindowClosing;

        try
        {
            _displayInfo.DpiChanged -= OnDpiChanged;
        }
        catch
        {
            // DisplayInformation 可能已在拆卸过程中释放
        }

        try
        {
            _coreWindow.VisibilityChanged -= OnVisibilityChanged;
        }
        catch
        {
            // CoreWindow 可能已在拆卸过程中释放
        }
    }

    // ================================================================
    // 窗口尺寸变化
    // ================================================================

    private void OnWindowSizeChanged(object sender, WindowSizeChangedEventArgs e)
    {
        // 窗口尺寸变化 → SwapChainPanel 可能也改变了尺寸
        // ViewportSurface.Resize() 内部从 ActualWidth/ActualHeight 读取最新尺寸
        // 并乘以 CompositionScale 得到物理像素尺寸，然后重建 DXGI 交换链
        _viewportSurface.Resize();
    }

    // ================================================================
    // DPI 变化
    // ================================================================

    private void OnDpiChanged(DisplayInformation sender, object args)
    {
        // DPI 变化 → CompositionScale 已自动更新
        // 触发视口尺寸重新计算即可
        _viewportSurface.Resize();
    }

    // ================================================================
    // 最小化/恢复
    // ================================================================

    private void OnVisibilityChanged(CoreWindow sender, VisibilityChangedEventArgs e)
    {
        if (e.Visible)
        {
            // 窗口从最小化恢复
            RenderingResumed?.Invoke();

            // 通知引擎线程恢复渲染
            _engineThread?.PostToEngine(() =>
            {
                // TODO(T16): 引擎恢复渲染循环
                // EngineAPI.ResumeRendering();
            });
        }
        else
        {
            // 窗口最小化 → 暂停渲染以节省 CPU/GPU 资源
            RenderingPaused?.Invoke();

            // 通知引擎线程暂停渲染
            _engineThread?.PostToEngine(() =>
            {
                // TODO(T16): 引擎暂停渲染循环
                // EngineAPI.PauseRendering();
            });
        }
    }

    // ================================================================
    // 窗口关闭
    // ================================================================

    private void OnWindowClosing(object sender, WindowEventArgs e)
    {
        // 先停止引擎线程，确保引擎在 UI 资源释放前完成清理
        // EngineThread.Stop() 有 CancellationTokenSource 保护，可安全多次调用
        _engineThread?.Stop();

        // 取消订阅所有事件（防止在 Dispose 后收到事件回调）
        Stop();
    }

    // ================================================================
    // IDisposable
    // ================================================================

    public void Dispose()
    {
        if (_disposed)
            return;
        _disposed = true;

        Stop();
    }
}
