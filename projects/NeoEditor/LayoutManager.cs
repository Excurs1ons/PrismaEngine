using WinUI.Dock;
using Microsoft.UI.Xaml.Controls;

namespace NeoEditor;

internal sealed class LayoutManager
{
    private readonly DockManager _dockManager;

    public SwapChainPanel SceneViewport { get; private set; }
    public WebView2 HierarchyWebView { get; private set; }
    public WebView2 InspectorWebView { get; private set; }
    public WebView2 ConsoleWebView { get; private set; }
    public WebView2 AssetsWebView { get; private set; }

    public LayoutManager(DockManager dockManager)
    {
        ArgumentNullException.ThrowIfNull(dockManager);
        _dockManager = dockManager;
    }

    public void SetupDefaultLayout()
    {
        ConfigurePanelDocuments();
    }

    private void ConfigurePanelDocuments()
    {
        foreach (var child in _dockManager.Children)
        {
            FindAndConfigureDocuments(child);
        }
    }

    private void FindAndConfigureDocuments(UIElement element)
    {
        if (element is LayoutPanel panel)
        {
            foreach (var panelChild in panel.Children)
            {
                FindAndConfigureDocuments(panelChild);
            }
        }
        else if (element is DocumentGroup group)
        {
            foreach (var item in group.Items)
            {
                if (item is Document doc)
                {
                    ConfigureDocument(doc);
                }
            }
        }
    }

    private void ConfigureDocument(Document doc)
    {
        switch (doc.Title)
        {
            case "Hierarchy":
                if (doc.Content is WebView2 wvHierarchy)
                    HierarchyWebView = wvHierarchy;
                break;
            case "Scene Viewport":
                if (doc.Content is SwapChainPanel scp)
                    SceneViewport = scp;
                break;
            case "Inspector":
                if (doc.Content is WebView2 wvInspector)
                    InspectorWebView = wvInspector;
                break;
            case "Console":
                if (doc.Content is WebView2 wvConsole)
                    ConsoleWebView = wvConsole;
                break;
            case "Assets":
                if (doc.Content is WebView2 wvAssets)
                    AssetsWebView = wvAssets;
                break;
        }
    }

    public void ResetLayout()
    {
        SetupDefaultLayout();
    }
}
