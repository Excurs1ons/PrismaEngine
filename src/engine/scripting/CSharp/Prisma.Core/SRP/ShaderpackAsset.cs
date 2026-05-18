using System.IO;
using System.IO.Compression;

namespace Prisma.SRP;

/// <summary>着色器包程序定义（一个 .vsh + .fsh 对）。</summary>
public class ShaderpackProgram
{
    public string Name { get; init; } = "";
    public string VertexSource { get; set; } = "";
    public string FragmentSource { get; set; } = "";
    public string? ComputeSource { get; set; }
}

/// <summary>Minecraft 维度。</summary>
public enum ShaderDimension
{
    Overworld,  // world0
    Nether,     // world-1
    End         // world1
}

/// <summary>
/// Iris/OptiFine 格式着色器包。
/// 从 zip 加载，支持 BSL v10 格式（.vsh/.fsh 入口 → #include 程序 .glsl）。
/// 支持 pre-1.15 旧版（直接 .vsh/.fsh）和 post-1.15 新版（dimension 分目录）。
/// </summary>
public class ShaderpackAsset
{
    /// <summary>程序名 → ShaderpackProgram</summary>
    public Dictionary<string, ShaderpackProgram> Programs { get; } = new();

    /// <summary>shaders.properties 原始内容</summary>
    public string? PropertiesContent { get; private set; }

    /// <summary>当前选中的维度</summary>
    public ShaderDimension Dimension { get; private set; } = ShaderDimension.Overworld;

    /// <summary>加载到内存中的 zip 存档</summary>
    public ZipArchive? Archive { get; private set; }

    /// <summary>从 zip 文件路径加载 shaderpack。</summary>
    public static ShaderpackAsset LoadFromZip(string zipPath, ShaderDimension dimension = ShaderDimension.Overworld)
    {
        var pack = new ShaderpackAsset();
        pack.Dimension = dimension;

        var archive = new ZipArchive(File.OpenRead(zipPath), ZipArchiveMode.Read);
        pack.Archive = archive;

        bool hasShadersDir = false;

        // 先收集所有 entry 路径
        var entryPaths = new HashSet<string>();
        foreach (var entry in archive.Entries)
            entryPaths.Add(entry.FullName.Replace('\\', '/'));

        // 读 shaders.properties
        var propsEntry = archive.GetEntry("shaders/shaders.properties")
                       ?? archive.GetEntry("shaders/shaders.properties");
        if (propsEntry != null)
        {
            using var r = new StreamReader(propsEntry.Open());
            pack.PropertiesContent = r.ReadToEnd();
        }

        // 维度目录名
        string dimDir = dimension switch
        {
            ShaderDimension.Overworld => "world0",
            ShaderDimension.Nether    => "world-1",
            ShaderDimension.End       => "world1",
            _ => "world0"
        };

        // 1) 找出所有 *unique program names* 来自 dimension 目录或 program 目录
        var programNames = new HashSet<string>();
        foreach (var ePath in entryPaths)
        {
            if (!ePath.StartsWith("shaders/")) continue;
            hasShadersDir = true;

            string rel = ePath["shaders/".Length..];

            // shaders/program/xxx.glsl, shaders/program/xxx.vsh, shaders/program/xxx.fsh
            if (rel.StartsWith("program/"))
            {
                string file = rel["program/".Length..];
                string name = Path.GetFileNameWithoutExtension(file);
                if (!string.IsNullOrEmpty(name))
                    programNames.Add(name);
            }

            // shaders/world0/xxx.vsh, etc
            if (rel.StartsWith("world-1/") || rel.StartsWith("world0/") || rel.StartsWith("world1/"))
            {
                string file = rel[(rel.IndexOf('/') + 1)..];
                string name = Path.GetFileNameWithoutExtension(file);
                if (!string.IsNullOrEmpty(name))
                    programNames.Add(name);
            }
        }

        if (!hasShadersDir)
            throw new InvalidDataException("Invalid shaderpack: no shaders/ directory found");

        // 2) 对每个程序，加载 .vsh + .fsh（优先维度目录，回退到 program/ 目录）
        foreach (string progName in programNames)
        {
            var prog = new ShaderpackProgram { Name = progName };

            // 加载 vertex shader
            string? vsPath = FindEntry(entryPaths, dimDir, progName, ".vsh");
            if (vsPath != null)
            {
                var entry = archive.GetEntry(vsPath);
                if (entry != null)
                    prog.VertexSource = GlslPreprocessor.PreprocessEntryPoint(archive, vsPath);
            }

            // 加载 fragment shader
            string? fsPath = FindEntry(entryPaths, dimDir, progName, ".fsh");
            if (fsPath != null)
            {
                var entry = archive.GetEntry(fsPath);
                if (entry != null)
                    prog.FragmentSource = GlslPreprocessor.PreprocessEntryPoint(archive, fsPath);
            }

            // 加载 compute shader
            string? csPath = FindEntry(entryPaths, dimDir, progName, ".csh");
            if (csPath != null)
            {
                var entry = archive.GetEntry(csPath);
                if (entry != null)
                {
                    using var r = new StreamReader(entry.Open());
                    prog.ComputeSource = r.ReadToEnd();
                }
            }

            // 如果没有 .vsh/.fsh，回退到 program/xxx.glsl（旧格式）
            if (string.IsNullOrEmpty(prog.VertexSource) && string.IsNullOrEmpty(prog.FragmentSource))
            {
                string glslPath = $"shaders/program/{progName}.glsl";
                if (entryPaths.Contains(glslPath))
                {
                    var (vs, fs) = GlslPreprocessor.SplitGlslProgram(archive, glslPath);
                    prog.VertexSource = vs;
                    prog.FragmentSource = fs;
                }
            }

            // 只保留至少有一个着色器的程序
            if (!string.IsNullOrEmpty(prog.VertexSource) || !string.IsNullOrEmpty(prog.FragmentSource) || !string.IsNullOrEmpty(prog.ComputeSource))
                pack.Programs[progName] = prog;
        }

        return pack;
    }

    /// <summary>
    /// 查找着色器 entry 点。优先级：
    /// 1. shaders/world{dim}/{name}.{ext}
    /// 2. shaders/program/{name}.{ext}
    /// 3. shaders/{name}.{ext}
    /// </summary>
    private static string? FindEntry(HashSet<string> entryPaths, string dimDir, string name, string ext)
    {
        // 1. Dimension 目录
        string dimPath = $"shaders/{dimDir}/{name}{ext}";
        if (entryPaths.Contains(dimPath))
            return dimPath;

        // 2. Program 目录
        string progPath = $"shaders/program/{name}{ext}";
        if (entryPaths.Contains(progPath))
            return progPath;

        // 3. 根 shaders 目录（旧格式）
        string rootPath = $"shaders/{name}{ext}";
        if (entryPaths.Contains(rootPath))
            return rootPath;

        return null;
    }

    /// <summary>
    /// 释放 zip 存档。
    /// </summary>
    public void Dispose()
    {
        Archive?.Dispose();
        Archive = null;
    }
}

/// <summary>
/// Minecraft 着色器 uniform 管理器。每帧更新 C++ GPU uniform 数据。
/// </summary>
public class MinecraftUniforms
{
    // World time
    public int WorldTime { get; set; }
    public int WorldDay { get; set; }
    public int MoonPhase { get; set; }

    // Sun / Moon
    public float SunAngle { get; set; }
    public float ShadowAngle { get; set; }
    public (float X, float Y, float Z) SunPosition { get; set; }
    public (float X, float Y, float Z) MoonPosition { get; set; }
    public (float X, float Y, float Z) ShadowLightPosition { get; set; }

    // Camera
    public (float X, float Y, float Z) CameraPosition { get; set; }
    public (float X, float Y, float Z) PreviousCameraPosition { get; set; }
    public float EyeAltitude { get; set; }

    // Weather
    public float RainStrength { get; set; }
    public float ThunderStrength { get; set; }
    public float Wetness { get; set; }

    // Fog
    public (float R, float G, float B) FogColor { get; set; }
    public (float R, float G, float B) SkyColor { get; set; }
    public float FogDensity { get; set; }

    // Frame
    public int FrameCounter { get; set; }
    public float FrameTime { get; set; }
    public float FrameTimeCounter { get; set; }

    // Matrices
    public float[] GbufferModelView { get; set; } = new float[16];
    public float[] GbufferModelViewInverse { get; set; } = new float[16];
    public float[] GbufferProjection { get; set; } = new float[16];
    public float[] GbufferProjectionInverse { get; set; } = new float[16];
    public float[] ShadowModelView { get; set; } = new float[16];
    public float[] ShadowProjection { get; set; } = new float[16];

    /// <summary>将当前 uniform 值推送到 C++ GPU 侧。</summary>
    public void PushToGPU()
    {
        // TODO: Implement GPU uniform buffer update
    }
}
