using WinUI.Dock;
using System.Diagnostics;

namespace NeoEditor;

internal sealed class EditorDockBehavior : IDockBehavior
{
    public void ActivateMainWindow()
    {
    }

    public void OnDocked(Document src, DockManager dest, DockTarget target)
    {
        Debug.WriteLine($"[Dock] '{src.ActualTitle}' docked to DockManager at {target}");
    }

    public void OnDocked(Document src, DocumentGroup dest, DockTarget target)
    {
        Debug.WriteLine($"[Dock] '{src.ActualTitle}' docked to DocumentGroup at {target}");
    }

    public void OnFloating(Document document)
    {
        Debug.WriteLine($"[Dock] '{document.ActualTitle}' is now floating");
    }
}
