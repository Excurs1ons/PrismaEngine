using System.Runtime.InteropServices;

namespace Prisma.SRP;

/// <summary>
/// 屏幕空间反射水面效果。
/// 通过 fragment shader + alpha blending 在屏幕下半部绘制反射水面。
/// 使用 push constants 控制水位线、时间和扭曲强度。
/// </summary>
public sealed class SSRWaterFeature : RendererFeature
{
    private const string VertSrc = @"#version 450 core
layout(location = 0) out vec2 vUV;
void main() {
    vec2 pos = vec2(float(gl_VertexIndex & 1), float((gl_VertexIndex >> 1) & 1)) * 2.0 - 1.0;
    vUV = pos * 0.5 + 0.5;
    gl_Position = vec4(pos, 0.0, 1.0);
}";

    private const string FragSrc = @"#version 450 core
layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;
layout(push_constant) uniform PushConstants {
    float waterLevel;
    float time;
    float distortion;
    float padding;
} pc;
void main() {
    if (vUV.y < pc.waterLevel) discard;

    float depth = vUV.y - pc.waterLevel;
    float normalizedDepth = depth / (1.0 - pc.waterLevel);

    float wave = sin(vUV.x * 25.0 + pc.time * 2.0) * pc.distortion;
    wave += cos(vUV.y * 12.0 + pc.time * 1.5) * pc.distortion * 0.6;
    wave += sin((vUV.x + vUV.y) * 15.0 + pc.time * 3.0) * pc.distortion * 0.3;

    float alpha = smoothstep(0.0, 0.15, depth) * 0.55;
    vec3 waterColor = mix(vec3(0.05, 0.25, 0.55), vec3(0.02, 0.15, 0.45), normalizedDepth);
    waterColor += wave * 0.15;

    float fresnel = 1.0 - abs(vUV.y * 2.0 - pc.waterLevel * 2.0 - 1.0);
    waterColor += fresnel * 0.08;

    outColor = vec4(waterColor, alpha);
}";

    [StructLayout(LayoutKind.Sequential)]
    private struct WaterParams
    {
        public float WaterLevel;
        public float Time;
        public float Distortion;
        public float Padding;
    }

    private Shader? _vs, _fs;
    private GraphicsPipeline? _pipeline;
    private float _timeAccum;
    private WaterParams _params = new() { WaterLevel = 0.55f, Distortion = 0.015f };

    public float WaterLevel
    {
        get => _params.WaterLevel;
        set => _params.WaterLevel = Math.Clamp(value, 0.1f, 0.9f);
    }

    public float DistortionStrength
    {
        get => _params.Distortion;
        set => _params.Distortion = Math.Clamp(value, 0f, 0.05f);
    }

    public float ElapsedTime => _timeAccum;

    public override void Create()
    {
        _vs = new Shader(VertSrc, ShaderStage.Vertex);
        _fs = new Shader(FragSrc, ShaderStage.Fragment);

        _pipeline = new GraphicsPipeline(_vs, _fs, new PipelineDesc
        {
            NumRenderTargets = 1,
            CullMode = 0,
            DepthTest = false,
            DepthWrite = false,
            BlendEnable = true,
            BlendColorWriteMask = 0xF,
        });
    }

    public override void Execute(CommandBuffer cmd)
    {
        if (_pipeline == null) return;

        _params.Time = _timeAccum;

        cmd.BindPipeline(_pipeline);
        unsafe
        {
            fixed (WaterParams* p = &_params)
                cmd.PushConstants(*p);
        }
        cmd.DrawFullScreenQuad();
    }

    public void Update(float dt)
    {
        _timeAccum += dt;
    }

    public override void Dispose()
    {
        _pipeline?.Dispose();
        _fs?.Dispose();
        _vs?.Dispose();
    }
}
