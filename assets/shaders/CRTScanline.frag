#version 450

/**
 * @brief CRT 扫描线 + 色散 + 亮度/对比度 后处理着色器
 *
 * 结合了像素完美整数倍上采样与 CRT 视觉效果。
 * 输入：像素完美离屏纹理（256×224）
 * 输出：交换链（整数倍缩放 + CRT 效果）
 *
 * 使用 PushConstants 传递 CRT 参数，
 * 使用 DescriptorSet (binding=0 texture, binding=1 sampler) 绑定输入纹理。
 */

layout(location = 0) in vec2 v_TexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform texture2D u_Texture;
layout(binding = 1) uniform sampler u_Sampler;

layout(push_constant) uniform CRTParams {
    float scanlineIntensity;    // 0.0 - 1.0，扫描线强度
    float chromaticAberration;  // 0.0 - 0.05，色散偏移（相对于逻辑分辨率宽度）
    float brightness;           // 0.0 - 2.0
    float contrast;             // 0.0 - 2.0
    float logicalWidth;         // 逻辑分辨率宽度（如 256.0）
    float logicalHeight;        // 逻辑分辨率高度（如 224.0）
} params;

void main() {
    // ── 1. 整数倍最近邻采样 ──
    // 将 UV 映射到逻辑像素网格，取最近邻像素中心
    vec2 texel = floor(v_TexCoord * vec2(params.logicalWidth, params.logicalHeight));
    vec2 nearestUV = (texel + 0.5) / vec2(params.logicalWidth, params.logicalHeight);

    // ── 2. 色散（Chromatic Aberration） ──
    // 在逻辑分辨率级别偏移 R 和 B 通道
    float chroma = params.chromaticAberration / params.logicalWidth;
    float r = texture(sampler2D(u_Texture, u_Sampler), nearestUV + vec2(chroma, 0.0)).r;
    float g = texture(sampler2D(u_Texture, u_Sampler), nearestUV).g;
    float b = texture(sampler2D(u_Texture, u_Sampler), nearestUV - vec2(chroma, 0.0)).b;
    vec3 color = vec3(r, g, b);

    // ── 3. 扫描线 ──
    // 基于 gl_FragCoord.y（输出像素坐标）产生交替明暗条纹
    float scanline = sin(gl_FragCoord.y * 3.14159265);
    scanline = clamp(scanline, 0.0, 1.0);
    scanline = 1.0 - (1.0 - scanline) * params.scanlineIntensity;
    color *= scanline;

    // ── 4. 亮度/对比度 ──
    color *= params.brightness;
    color = (color - 0.5) * params.contrast + 0.5;

    outColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
