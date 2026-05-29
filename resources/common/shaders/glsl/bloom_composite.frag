#version 450

// ============================================================================
// PrismaEngine — Bloom 合成片段着色器
// 将原始 HDR 场景颜色与模糊后的 Bloom 光晕混合，
// 并应用色调映射和伽马校正输出最终 LDR 颜色。
//
// 配合 fullscreen.vert 或 FullscreenTri.vert 使用
// (v_TexCoord 由顶点着色器通过 gl_VertexIndex 生成)
// ============================================================================

layout(location = 0) in vec2 v_TexCoord;
layout(location = 0) out vec4 outColor;

// 原始 HDR 场景颜色
layout(binding = 0) uniform sampler2D u_OriginalTexture;

// 模糊后的 Bloom 光晕
layout(binding = 1) uniform sampler2D u_BloomTexture;

layout(push_constant) uniform BloomCompositeParams {
    float u_Intensity;   // Bloom 强度倍数
    float u_Gamma;       // 伽马值 (默认 2.2)
} params;

void main() {
    vec4 original = texture(u_OriginalTexture, v_TexCoord);
    vec4 bloom = texture(u_BloomTexture, v_TexCoord);

    // Bloom 合成: 原始 HDR + Bloom (叠加混合)
    vec3 hdrResult = original.rgb + bloom.rgb * params.u_Intensity;

    // Reinhard 色调映射: HDR -> LDR
    vec3 mapped = hdrResult / (hdrResult + vec3(1.0));

    // 伽马校正
    vec3 gammaCorrected = pow(mapped, vec3(1.0 / params.u_Gamma));

    outColor = vec4(gammaCorrected, 1.0);
}
