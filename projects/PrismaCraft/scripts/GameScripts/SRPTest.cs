using Prisma;
using Prisma.SRP;

namespace GameScripts;

internal static class SRPTest
{
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

    const string CSsrc = @"
#version 450 core
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
layout(rgba8, binding = 0) uniform image2D outputImg;
void main() {
    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
    vec2 uv = vec2(pos) / vec2(imageSize(outputImg));
    vec3 c = 0.5 + 0.5 * cos(uv.xyx + vec3(0, 2, 4));
    imageStore(outputImg, pos, vec4(c, 1.0));
}";

    public static void Run()
    {
        Debug.Log("[SRPTest] Starting end-to-end SRP test...");

        var vs = new Shader(VertSrc, ShaderStage.Vertex);
        var fs = new Shader(FragSrc, ShaderStage.Fragment);
        if (vs.Handle == 0 || fs.Handle == 0)
        {
            Debug.LogError("[SRPTest] Shader compilation failed!");
            return;
        }
        Debug.Log($"[SRPTest] Shaders compiled: vs={vs.Handle}, fs={fs.Handle}");

        var pipeline = new GraphicsPipeline(vs, fs, new PipelineDesc
        {
            NumRenderTargets = 1,
            DepthStencilFormat = 50,
            CullMode = 0,
            DepthTest = false,
            DepthWrite = false,
            BlendColorWriteMask = 0xF,
        });
        if (pipeline.Handle == 0)
        {
            Debug.LogError("[SRPTest] Pipeline creation failed!");
            return;
        }
        Debug.Log($"[SRPTest] Pipeline created: {pipeline.Handle}");

        var cs = new Shader(CSsrc, ShaderStage.Compute);
        if (cs.Handle != 0)
        {
            var computePipeline = new ComputePipeline(cs, 0);
            Debug.Log($"[SRPTest] Compute pipeline created: {computePipeline.Handle}");
        }

        ScriptEngine.OnRenderCallback = dt =>
        {
            var cmd = new CommandBuffer();
            cmd.BeginFrame();
            cmd.BindPipeline(pipeline);
            cmd.SetViewport(0, 0, 1280, 720);
            cmd.SetScissor(0, 0, 1280, 720);
            cmd.DrawFullScreenQuad();
            cmd.EndFrame();
        };

        Debug.Log("[SRPTest] SRP pipeline ready! Rendering fullscreen quad every frame.");
    }
}
