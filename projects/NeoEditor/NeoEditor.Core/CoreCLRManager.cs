namespace NeoEditor.Core;

public sealed class CoreCLRManager
{
    public bool IsInitialized { get; private set; }

    public void Initialize()
    {
        IsInitialized = true;
    }

    public void Shutdown()
    {
        IsInitialized = false;
    }
}
