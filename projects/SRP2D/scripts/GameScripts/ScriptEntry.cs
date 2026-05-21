using System.Runtime.InteropServices;
using Prisma;
using Prisma.SRP;

namespace SRP2D;

public static class ScriptEntry
{
    private static SRP2DRenderPipeline? _pipeline;
    private static World? _world;

    [UnmanagedCallersOnly(EntryPoint = "Bootstrap")]
    public static void Bootstrap(IntPtr apiPtr)
    {
        unsafe { Interop.Init((PrismaAPI*)apiPtr); }

        _world = new World();
        World.Active = _world;

        try
        {
            var registryType = Type.GetType("Prisma.Generated.ScriptRegistry, GameScripts")
                            ?? Type.GetType("Prisma.Generated.ScriptRegistry, Prisma.Core");
            registryType?.GetMethod("RegisterAll")?.Invoke(null, null);
        }
        catch { }

        _pipeline = new SRP2DRenderPipeline();
        _pipeline.Build();
        ScriptEngine.OnRenderCallback = (_) => _pipeline.Render();

        var sceneSetup = Node.Create("__SceneSetup__");
        sceneSetup.AddScript<SceneSetup>();

        Light2D.AmbientColor = new Vector3(0.1f, 0.1f, 0.15f);

        Debug.Log("SRP2D Pipeline initialized.");
    }

    [UnmanagedCallersOnly(EntryPoint = "OnFrame")]
    public static void OnFrame(float dt)
    {
        _world?.Step(dt);
        _pipeline?.WaterSSR.Update(dt);
    }

    [UnmanagedCallersOnly(EntryPoint = "OnRender")]
    public static void OnRender(float dt)
    {
    }
}
