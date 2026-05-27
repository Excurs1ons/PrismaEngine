using WinUI.Dock;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace NeoEditor;

internal sealed class EditorDockAdapter : IDockAdapter
{
    public void OnCreated(Document document)
    {
    }

    public void OnCreated(DocumentGroup group, Document? draggedDocument)
    {
    }

    public object? GetFloatingWindowTitleBar(Document? draggedDocument)
    {
        return null;
    }
}
