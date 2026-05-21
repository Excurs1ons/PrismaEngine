using System.Runtime.InteropServices;

namespace Prisma.SRP;

/// <summary>
/// 2D 屏幕空间反射水面效果 (RendererFeature2D)。
/// 先 BlitRenderTarget 捕获当前帧到纹理，再用 fragment shader 采样反射。
/// 通过 push constants 控制水位线、时间和扭曲强度。
/// </summary>
public sealed class SSRWaterFeature2D : RendererFeature2D
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
layout(binding = 1) uniform sampler2D sceneTex;
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

    float wave = sin(vUV.x * 25.0 + pc.time * 2.0) * pc.distortion * 0.8;
    wave += cos(vUV.y * 12.0 + pc.time * 1.5) * pc.distortion * 0.5;

    vec2 reflectedUV = vec2(vUV.x + wave, pc.waterLevel - depth);
    reflectedUV = clamp(reflectedUV, 0.0, 1.0);
    vec3 reflectedColor = texture(sceneTex, reflectedUV).rgb;

    float reflectionStrength = 0.4 + normalizedDepth * 0.25;
    vec3 waterColor = mix(
        vec3(0.02, 0.12, 0.35),
        vec3(0.05, 0.22, 0.55),
        normalizedDepth
    );

    vec3 finalColor = mix(waterColor, reflectedColor, reflectionStrength);

    float fresnel = 1.0 - abs(vUV.y * 2.0 - pc.waterLevel * 2.0 - 1.0);
    finalColor += fresnel * 0.06;

    float alpha = smoothstep(0.0, 0.12, depth) * 0.6;
    outColor = vec4(finalColor, alpha);
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
    private Texture? _sceneTex;
    private Sampler? _sceneSampler;
    private float _timeAccum;
    private WaterParams _params = new() { WaterLevel = 0.55f, Distortion = 0.015f };
    private int _texW = 1280, _texH = 720;

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

    public void SetResolution(int w, int h) { _texW = w; _texH = h; }

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

        _sceneTex = new Texture(_texW, _texH, 37);
        _sceneSampler = new Sampler(minFilter: 1, magFilter: 1, addressU: 1, addressV: 1);
    }

    public override void Execute(CommandBuffer cmd)
    {
        if (_pipeline == null || _sceneTex == null || _sceneSampler == null) return;

        cmd.BlitRenderTarget(_sceneTex);

        _params.Time = _timeAccum;

        cmd.BindPipeline(_pipeline);
        cmd.BindTexture(1, _sceneTex, _sceneSampler);
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
        _sceneTex?.Dispose();
        _sceneSampler?.Dispose();
        _fs?.Dispose();
        _vs?.Dispose();
    }
}
