using System;
using System.Runtime.InteropServices;
using Prisma;
using Prisma.SRP;

namespace GameScripts;

/// <summary>
/// SRP 端到端测试：编译着色器 → 创建管线 → 录制命令 → 渲染全屏四边形。
/// 在 Bootstrap 时自动运行，输出渲染结果。
/// </summary>
internal static class SRPTest
{
    // 最小 GLSL 着色器（#version 450 core，与现有引擎兼容）
    const string VertSrc = @"
#version 450 core
layout(location = 0) out vec2 uv;
void main() {
    vec2 pos = vec2(float(gl_VertexIndex & 1), float((gl_VertexIndex >> 1) & 1)) * 2.0 - 1.0;
    uv = pos * 0.5 + 0.5;
    gl_Position = vec4(pos, 0.0, 1.0);
}";

    const string FragSrc = @"
#version 450 core
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
void main() {
    vec3 c = 0.5 + 0.5 * cos(uv.xyx + vec3(0, 2, 4));
    color = vec4(c, 1.0);
}";

    // Minecraft 风格 gbuffers_terrain（带光照计算）
    const string TerrainVert = @"
#version 450 core
layout(location = 0) in vec3 pos;
layout(location = 2) in vec2 texcoord;
layout(location = 3) in vec2 lmcoord;
layout(location = 0) out vec2 texCoord;
layout(location = 1) out vec2 lightCoord;
void main() {
    texCoord = texcoord;
    lightCoord = lmcoord;
    gl_Position = ftransform();
}";

    const string TerrainFrag = @"
#version 450 core
layout(location = 0) in vec2 texCoord;
layout(location = 1) in vec2 lightCoord;
layout(location = 0) out vec4 color;
uniform sampler2D gtexture;
uniform sampler2D lightmap;
void main() {
    vec4 tex = texture(gtexture, texCoord);
    vec2 lm = texture(lightmap, lightCoord).rg;
    color = tex * vec4(lm, lm.x, 1.0);
}";

    // 全屏复合 Pass
    const string CompositeFrag = @"
#version 450 core
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
uniform sampler2D colortex0;
void main() {
    color = texture(colortex0, uv);
}";

    public static void Run()
    {
        Debug.Log("[SRPTest] Starting end-to-end SRP test...");

        unsafe
        {
            // 1. 编译着色器
            uint vs = CompileVertex(VertSrc);
            uint fs = CompileFragment(FragSrc);
            if (vs == 0 || fs == 0)
            {
                Debug.LogError("[SRPTest] Shader compilation failed!");
                return;
            }
            Debug.Log($"[SRPTest] Shaders compiled: vs={vs}, fs={fs}");

            // 2. 创建管线
            var desc = new SRPPipelineDesc
            {
                VertexShader = vs,
                FragmentShader = fs,
                NumRenderTargets = 1,
                DepthStencilFormat = 50, // D32_Float
                CullMode = 0,  // None
                DepthTest = 0,
                DepthWrite = 0,
                BlendColorWriteMask = 0xF,
            };
            desc.RenderTargetFormats[0] = 0; // RGBA8_UNorm

            uint pipeline;
            pipeline = Interop.API.SrpCreatePipeline(&desc);
            if (pipeline == 0)
            {
                Debug.LogError("[SRPTest] Pipeline creation failed!");
                return;
            }
            Debug.Log($"[SRPTest] Pipeline created: {pipeline}");

            // 3. 注册 SRP 渲染回调
            ScriptEngine.OnRenderCallback = dt =>
            {
                unsafe
                {
                    // 获取当前帧命令缓冲（引擎 BeginFrame 已启动）
                    Interop.API.SrpBeginFrame();

                    // 绑定管线
                    Interop.API.SrpCmdBindPipeline(pipeline);

                    // 视口
                    Interop.API.SrpCmdSetViewport(0, 0, 1280, 720);
                    Interop.API.SrpCmdSetScissor(0, 0, 1280, 720);

                    // 绘制全屏四边形（4 顶点）
                    Interop.API.SrpCmdDrawFullScreenQuad();

                    Interop.API.SrpEndFrame();
                }
            };

            Debug.Log("[SRPTest] SRP pipeline ready! Rendering fullscreen quad every frame.");
        }
    }

    private static unsafe uint CompileVertex(string src)
    {
        fixed (byte* ptr = System.Text.Encoding.UTF8.GetBytes(src))
        {
            return Interop.API.SrpCreateShader(ptr, (uint)src.Length, (uint)SRPShaderStage.Vertex);
        }
    }

    private static unsafe uint CompileFragment(string src)
    {
        fixed (byte* ptr = System.Text.Encoding.UTF8.GetBytes(src))
        {
            return Interop.API.SrpCreateShader(ptr, (uint)src.Length, (uint)SRPShaderStage.Fragment);
        }
    }
}
