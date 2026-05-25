using System.Diagnostics;
using System.Threading;
using Microsoft.UI.Input;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;
using NeoEditor.Core;
using NeoEditor.Core.Interop;
using Windows.System;

namespace NeoEditor;

public sealed partial class MainWindow : Window
{
    private const int DefaultWidth = 1600;
    private const int DefaultHeight = 900;

    private ViewportSurface? _viewportSurface;
    private EngineThread? _engineThread;
    private LayoutManager? _layoutManager;
    private SwapChainPanel? _sceneViewport;
    private WebUIService? _webUIService;
    private InputRouter? _inputRouter;
    private Timer? _eventSyncTimer;
    private int _eventFpsCounter;
    private DateTime _eventFpsLastReset = DateTime.UtcNow;
    private readonly Dictionary<string, WebUIBridge> _webBridges = new();

    public MainWindow()
    {
        InitializeComponent();

        Title = "NeoEditor - PrismaEngine";

        var hWnd = WinRT.Interop.WindowNative.GetWindowHandle(this);
        var windowId = Microsoft.UI.Win32Interop.GetWindowIdFromWindow(hWnd);
        var appWindow = Microsoft.UI.Windowing.AppWindow.GetFromWindowId(windowId);

        appWindow.Resize(new Windows.Graphics.SizeInt32(DefaultWidth, DefaultHeight));

        MainDockManager.Adapter = new EditorDockAdapter();
        MainDockManager.Behavior = new EditorDockBehavior();

        // Event subscriptions moved to OnLoaded after LayoutManager sets up
        // the dock panels; see OnLoaded for SceneViewport subscriptions.
        Loaded += OnLoaded;
        Closed += OnClosed;
    }

    // ================================================================
    // Layout & Viewport lifecycle
    // ================================================================

    private void OnLoaded(object sender, RoutedEventArgs e)
    {
        try
        {
            _layoutManager = new LayoutManager(MainDockManager);
            _layoutManager.SetupDefaultLayout();

            // Cache SceneViewport reference from LayoutManager for
            // event subscriptions and ViewportSurface construction.
            _sceneViewport = _layoutManager.SceneViewport;
            if (_sceneViewport == null)
            {
                System.Diagnostics.Debug.WriteLine(
                    "[NeoEditor] SceneViewport not found in dock layout");
                return;
            }

            // Subscribe viewport events after LayoutManager setup
            _sceneViewport.SizeChanged += OnViewportSizeChanged;
            _sceneViewport.CompositionScaleChanged += OnViewportCompositionScaleChanged;

            // Initialize input routing (T14): WinUI3 events → Engine thread via message queue
            if (_engineThread != null)
            {
                _inputRouter = new InputRouter(_engineThread.UIToEngine);

                _sceneViewport.KeyDown += (s, e) =>
                {
                    _inputRouter.OnKeyDown(e.Key);
                    e.Handled = true;
                };
                _sceneViewport.KeyUp += (s, e) =>
                {
                    _inputRouter.OnKeyUp(e.Key);
                    e.Handled = true;
                };
                _sceneViewport.PointerMoved += (s, e) =>
                {
                    var pt = e.GetCurrentPoint(_sceneViewport);
                    _inputRouter.OnPointerMoved((int)pt.Position.X, (int)pt.Position.Y);
                    e.Handled = true;
                };
                _sceneViewport.PointerPressed += (s, e) =>
                {
                    _inputRouter.OnPointerPressed(e.GetCurrentPoint(_sceneViewport));
                    e.Handled = true;
                };
                _sceneViewport.PointerReleased += (s, e) =>
                {
                    _inputRouter.OnPointerReleased(e.GetCurrentPoint(_sceneViewport));
                    e.Handled = true;
                };
                _sceneViewport.PointerWheelChanged += (s, e) =>
                {
                    var pt = e.GetCurrentPoint(_sceneViewport);
                    _inputRouter.OnPointerWheelChanged((int)pt.Properties.MouseWheelDelta);
                    e.Handled = true;
                };
                _sceneViewport.LostFocus += (s, e) =>
                {
                    _inputRouter.ReleaseAllKeys();
                };
            }
            else
            {
                // Fallback: no engine thread, simple key handling
                _sceneViewport.KeyDown += OnViewportKeyDown;
                _sceneViewport.GotFocus += OnViewportGotFocus;
            }

            // Create and initialize the ViewportSurface
            // The SwapChainPanel (_sceneViewport) now has a valid size
            _viewportSurface = new ViewportSurface(_sceneViewport);

            // Pass Vulkan LUID if engine is available and has exported one
            // For now, create D3D11 device on the default adapter
            _viewportSurface.Initialize(vulkanLuid: null);

            // Connect engine queue for shared texture delivery (future T19)
            if (_engineThread != null)
            {
                _viewportSurface.SetEngineQueue(_engineThread.EngineToUI);
            }

            // Initialize WindowManager (T15): 集中管理窗口生命周期事件
            _windowManager = new WindowManager(
                this, _sceneViewport, _viewportSurface, _engineThread);
            _windowManager.RenderingPaused += OnRenderingPaused;
            _windowManager.RenderingResumed += OnRenderingResumed;
            _windowManager.Start();
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine(
                $"[NeoEditor] ViewportSurface init failed: {ex.Message}");
        }

        // Start HTTP service for WebUI communication via WebView2 panels
        try
        {
            _webUIService = new WebUIService(port: 8080);
            _webUIService.Start();
            Debug.WriteLine(
                "[NeoEditor] WebUI HTTP service started on port 8080");
        }
        catch (Exception ex)
        {
            Debug.WriteLine(
                $"[NeoEditor] Failed to start HTTP service: {ex.Message}");
            Debug.WriteLine(
                "[NeoEditor] On Windows, try: netsh http add urlacl http://localhost:8080/ user=Everyone");
        }

        // Initialize WebView2 panels for WebUI panel hosting
        try
        {
            _ = InitializeWebView2Panels();
        }
        catch (Exception ex)
        {
            Debug.WriteLine($"[NeoEditor] WebView2 init failed: {ex.Message}");
            Debug.WriteLine("[NeoEditor] Ensure WebView2 runtime is installed");
        }

        // Start event sync: Engine ↔ WebUI 双向事件同步
        StartEventSync();
    }

    // ================================================================
    // Event Sync (T17): Engine ↔ WebUI 双向事件同步
    // ================================================================

    /// <summary>
    /// 启动事件同步循环。
    /// 连接引擎线程事件到 EventBus，并启动周期性状态推送（500ms）。
    /// </summary>
    private void StartEventSync()
    {
        // 连接引擎线程事件 (当引擎运行时)
        if (_engineThread != null)
        {
            // 引擎 FPS 更新 → EventBus 广播
            _engineThread.FpsUpdated += fps =>
            {
                EventBus.Instance.PublishEngineStatus(
                    fps: (int)fps,
                    gpu: "Prisma Vulkan",
                    scene: "Main Scene",
                    objects: 100);
            };

            // 引擎日志 → 也可通过 EventBus 转发 (预留)
            _engineThread.EngineLogReceived += log =>
            {
                // TODO: T20 将引擎日志推送到 WebUI Console 面板
                Debug.WriteLine($"[Engine] {log}");
            };
        }

        // 周期性事件推送 (500ms): 确保 WebUI 状态栏持续更新
        // 当引擎线程激活后，此定时器可作为补充保障
        _eventSyncTimer = new Timer(OnEventSyncTick, null,
            dueTime: TimeSpan.FromSeconds(1),
            period: TimeSpan.FromMilliseconds(500));

        Debug.WriteLine("[MainWindow] Event sync started (500ms interval)");
    }

    /// <summary>
    /// 定时器回调: 发布引擎状态事件。
    /// 计算简单 FPS 计数器，推送缓存的状态快照。
    /// </summary>
    private void OnEventSyncTick(object? state)
    {
        // 简单 FPS 计算: 每 500ms tick = 2 ticks/sec
        var now = DateTime.UtcNow;
        Interlocked.Increment(ref _eventFpsCounter);

        double elapsed = (now - _eventFpsLastReset).TotalSeconds;
        int fps;
        if (elapsed >= 1.0)
        {
            int count = Interlocked.Exchange(ref _eventFpsCounter, 0);
            fps = (int)Math.Round(count / elapsed);
            _eventFpsLastReset = now;
        }
        else
        {
            fps = _eventFpsCounter * 2; // estimate
        }

        EventBus.Instance.PublishEngineStatus(
            fps: Math.Min(fps, 60),
            gpu: "N/A (editor)",
            scene: "NeoEditor",
            objects: 0);
    }

    private void OnClosed(object sender, WindowEventArgs args)
    {
        // 停止事件同步定时器
        _eventSyncTimer?.Dispose();
        _eventSyncTimer = null;

        // Dispose all WebUIBridge instances before WebView2 controls are released
        foreach (var bridge in _webBridges.Values)
        {
            bridge.Dispose();
        }
        _webBridges.Clear();

        _inputRouter?.Dispose();

        // Dispose WindowManager before ViewportSurface to ensure
        // all window event handlers are unsubscribed first
        _windowManager?.Dispose();
        _windowManager = null;

        if (_viewportSurface != null)
        {
            _viewportSurface.Dispose();
            _viewportSurface = null;
        }

        _webUIService?.Dispose();
        _engineThread?.Dispose();
    }

    // ================================================================
    // WebView2 panel initialization
    // ================================================================

    /// <summary>
    /// 并行初始化所有 WebView2 面板，提升启动速度。
    /// 每个面板导航到 WebUI 服务的对应路径。
    /// </summary>
    private async Task InitializeWebView2Panels()
    {
        // 面板路径映射: WebView2 名称 → HTTP 路径
        var panels = new (WebView2 View, string Name, string Path)[]
        {
            (HierarchyWebView,  "hierarchy",  "/hierarchy"),
            (InspectorWebView,  "inspector",  "/inspector"),
            (ConsoleWebView,    "console",    "/console"),
            (AssetsWebView,     "assets",     "/assets"),
        };

        var tasks = panels.Select(p => InitializeWebView(p.View, p.Name, p.Path));
        await Task.WhenAll(tasks);

        Debug.WriteLine($"[NeoEditor] All {_webBridges.Count} WebView2 panels initialized");
    }

    private async Task InitializeWebView(WebView2 webView, string panelName, string panelPath)
    {
        try
        {
            // CoreWebView2 初始化是异步的，需等待完成
            await webView.EnsureCoreWebView2Async();

            var core = webView.CoreWebView2!;

            // 安全配置: 禁用自动填充/脚本对话框，启用 WebMessage
            core.Settings.IsScriptEnabled = true;
            core.Settings.IsWebMessageEnabled = true;
            core.Settings.AreDefaultScriptDialogsEnabled = false;
            core.Settings.IsPasswordAutosaveEnabled = false;
            core.Settings.IsGeneralAutofillEnabled = false;

            // 导航到 WebUI 服务对应路径
            // WebUI 通过 HTTP 调用 REST API (已由 T11 WebUIService 实现)
            core.Navigate($"http://localhost:8080{panelPath}");

            // 创建 WebUIBridge 并注册
            var bridge = new WebUIBridge(webView);
            bridge.Initialize();
            _webBridges[panelName] = bridge;

            Debug.WriteLine($"[NeoEditor] WebView2 panel '{panelName}' navigated to {panelPath}");
        }
        catch (Exception ex)
        {
            Debug.WriteLine($"[NeoEditor] WebView2 panel '{panelName}' init failed: {ex.Message}");
            Debug.WriteLine("[NeoEditor] Ensure WebView2 Runtime is installed (https://developer.microsoft.com/en-us/microsoft-edge/webview2/)");
        }
    }

    // ================================================================
    // Viewport resize
    // ================================================================

    private void OnViewportSizeChanged(object sender, SizeChangedEventArgs e)
    {
        _viewportSurface?.Resize();
    }

    private void OnViewportCompositionScaleChanged(SwapChainPanel sender, object args)
    {
        _viewportSurface?.Resize();
    }

    // ================================================================
    // Window lifecycle (pause/resume from WindowManager)
    // ================================================================

    private void OnRenderingPaused()
    {
        _viewportSurface?.PauseRendering();
    }

    private void OnRenderingResumed()
    {
        _viewportSurface?.ResumeRendering();
    }

    // ================================================================
    // Key / focus
    // ================================================================

    private void OnViewportKeyDown(object sender, KeyRoutedEventArgs e)
    {
        e.Handled = false;
    }

    private void OnViewportGotFocus(object sender, RoutedEventArgs e)
    {
    }
}
