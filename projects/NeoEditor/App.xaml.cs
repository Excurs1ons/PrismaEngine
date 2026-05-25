using Microsoft.UI.Xaml;

namespace NeoEditor;

public partial class App : Application
{
    private Window? m_window;

    protected override void OnLaunched(Microsoft.UI.Xaml.LaunchActivatedEventArgs args)
    {
        m_window = new MainWindow();
        m_window.Activate();
    }
}
