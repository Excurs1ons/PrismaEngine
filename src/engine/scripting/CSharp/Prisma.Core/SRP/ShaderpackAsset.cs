using System.IO;
using System.IO.Compression;

namespace Prisma.SRP;

/// <summary>
/// Minecraft 着色器包定义（由 shaderpack zip 解析而来）。
/// </summary>
public class ShaderpackProgram
{
    public string Name { get; init; } = "";
    public string VertexSource { get; set; } = "";
    public string FragmentSource { get; set; } = "";
    public string? ComputeSource { get; set; }
    public Dictionary<string, string> Defines { get; init; } = new();
}

/// <summary>
/// 解析 Iris/OptiFine 格式的着色器包 zip。
/// 提取所有程序（gbuffers_terrain, composite, shadow...）的 GLSL 源码。
/// </summary>
public class ShaderpackAsset
{
    /// <summary>程序名 → ShaderpackProgram</summary>
    public Dictionary<string, ShaderpackProgram> Programs { get; } = new();

    /// <summary>shaders.properties 原始内容</summary>
    public string? PropertiesContent { get; private set; }

    /// <summary>从 zip 文件路径加载 shaderpack</summary>
    public static ShaderpackAsset LoadFromZip(string zipPath)
    {
        var pack = new ShaderpackAsset();

        using var archive = ZipFile.OpenRead(zipPath);
        bool hasShadersDir = false;

        foreach (var entry in archive.Entries)
        {
            // 只处理 shaders/ 目录下的文件
            if (!entry.FullName.StartsWith("shaders/") || entry.Name == "") continue;
            hasShadersDir = true;

            var relativePath = entry.FullName.Substring("shaders/".Length);

            // shaders.properties
            if (relativePath == "shaders.properties" || relativePath == "/shaders.properties")
            {
                using var reader = new StreamReader(entry.Open());
                pack.PropertiesContent = reader.ReadToEnd();
                continue;
            }

            // 提取扩展名
            string ext = Path.GetExtension(entry.Name).ToLowerInvariant();
            string programName = Path.GetFileNameWithoutExtension(entry.Name);

            // .vsh / .fsh / .csh
            using var stream = entry.Open();
            using var memStream = new MemoryStream();
            stream.CopyTo(memStream);
            string source = System.Text.Encoding.UTF8.GetString(memStream.ToArray());

            if (!pack.Programs.TryGetValue(programName, out var prog))
            {
                prog = new ShaderpackProgram { Name = programName };
                pack.Programs[programName] = prog;
            }

            switch (ext)
            {
                case ".vsh": prog.VertexSource = source; break;
                case ".fsh": prog.FragmentSource = source; break;
                case ".csh": prog.ComputeSource = source; break;
            }
        }

        if (!hasShadersDir)
            throw new InvalidDataException("Invalid shaderpack: no shaders/ directory found");

        return pack;
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
