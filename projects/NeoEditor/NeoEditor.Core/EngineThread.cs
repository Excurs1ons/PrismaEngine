using System.Runtime.InteropServices;
using System.Threading;
using NeoEditor.Core.Interop;

namespace NeoEditor.Core;

public sealed class EngineThread : IDisposable
{
    private Thread?                 _engineThread;
    private CancellationTokenSource? _cts;
    private EngineHandle?           _engineHandle;
    private readonly ManualResetEventSlim _wakeEvent = new(false);

    public bool IsRunning       => _engineThread?.IsAlive ?? false;
    public bool IsEngineLoaded  => _engineHandle != null;

    public EngineToUIQueue EngineToUI { get; } = new();
    public UIToEngineQueue UIToEngine { get; } = new();

    public event Action<string>? EngineLogReceived;
#pragma warning disable CS0067 // 预留，T16 将接入引擎主循环
    public event Action<float>?  FpsUpdated;
#pragma warning restore CS0067
    public event Action?         EngineCrashed;

    public void Start()
    {
        if (IsRunning) return;

        _cts = new CancellationTokenSource();
        _engineThread = new Thread(EngineLoop)
        {
            Name = "PrismaEngine",
            IsBackground = true,
        };
        _engineThread.Start();
    }

    public void Stop()
    {
        if (_cts == null) return;

        _cts.Cancel();
        _wakeEvent.Set();

        if (_engineThread != null && _engineThread.IsAlive)
        {
            if (!_engineThread.Join(5000))
            {
                EngineLogReceived?.Invoke(
                    "[EngineThread] Thread did not exit within 5s, abandoning.");
            }
        }

        if (_engineHandle != null)
        {
            _engineHandle.Dispose();
            _engineHandle = null;
        }

        _cts.Dispose();
        _cts = null;
    }

    public void PostToEngine(Action action)
    {
        UIToEngine.Enqueue(action);
        _wakeEvent.Set();
    }

    private void EngineLoop()
    {
        try
        {
            EngineLogReceived?.Invoke("[EngineThread] Loading Prisma.dll...");

            IntPtr namePtr = IntPtr.Zero;
            try
            {
                namePtr = Marshal.StringToCoTaskMemAnsi("NeoEditor");
                var config = new EngineCreateInfo
                {
                    Name                       = namePtr,
                    Headless                   = 1,
                    MinLogLevel                = 0,
                    PresentMode                = 0,
                    MaxFPS                     = 0,
                    RefreshAssetDatabaseOnStartup = 0,
                };

                _engineHandle = new EngineHandle("Prisma.dll", config);
            }
            finally
            {
                if (namePtr != IntPtr.Zero)
                    Marshal.FreeCoTaskMem(namePtr);
            }

            EngineLogReceived?.Invoke("[EngineThread] Engine loaded, initializing...");

            int initResult = _engineHandle.Initialize();
            if (initResult != 0)
            {
                EngineLogReceived?.Invoke(
                    $"[EngineThread] Engine Initialize failed: {initResult}");
                return;
            }

            EngineLogReceived?.Invoke("[EngineThread] Engine initialized successfully");

            while (!_cts!.Token.IsCancellationRequested)
            {
                UIToEngine.Drain(action =>
                {
                    try
                    {
                        action();
                    }
                    catch (Exception ex)
                    {
                        EngineLogReceived?.Invoke(
                            $"[EngineThread] Command error: {ex.Message}");
                    }
                });

                _wakeEvent.Wait(50);
                _wakeEvent.Reset();
            }
        }
        catch (OperationCanceledException)
        {
        }
        catch (Exception ex)
        {
            EngineLogReceived?.Invoke(
                $"[EngineThread] Fatal error: {ex.Message}");
            EngineCrashed?.Invoke();
        }
        finally
        {
            try
            {
                _engineHandle?.Shutdown();
                EngineLogReceived?.Invoke("[EngineThread] Engine shutdown complete");
            }
            catch (Exception ex)
            {
                EngineLogReceived?.Invoke(
                    $"[EngineThread] Shutdown error: {ex.Message}");
            }
        }
    }

    public void Dispose()
    {
        Stop();
        _wakeEvent.Dispose();
        _cts?.Dispose();
    }
}
