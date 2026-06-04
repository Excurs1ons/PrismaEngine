#version 460
// ============================================================================
// PrismaEngine — Bloom 合成片段着色器（单通道）
// 对输入纹理进行 5×5 高斯模糊并将结果叠加到原图上。
// 无需临时纹理或多通道渲染——在一遍绘制内完成。
//
// 配合 FullscreenTri.vert 使用
// (v_TexCoord 由顶点着色器通过 gl_VertexIndex 生成)
// ============================================================================

layout(location = 0) in vec2 v_TexCoord;
layout(location = 0) out vec4 out_Color;

layout(binding = 0) uniform sampler2D u_Original;

layout(push_constant) uniform BloomCompositeParams {
    float u_Intensity; // Bloom 强度
};

void main() {
    vec2 texelSize = 1.0 / textureSize(u_Original, 0);
    vec4 original = texture(u_Original, v_TexCoord);

    // 5-tap Gaussian 权重 (Pascal 三角形第 5 行归一化: 1,4,6,4,1)
    const float w[5] = float[](
        0.0625,  // 1/16
        0.25,    // 4/16
        0.375,   // 6/16
        0.25,    // 4/16
        0.0625   // 1/16
    );

    // 5×5 高斯模糊 (25 纹理采样)
    vec4 blur = vec4(0.0);
    for (int y = -2; y <= 2; y++) {
        for (int x = -2; x <= 2; x++) {
            vec2 uv = v_TexCoord + vec2(float(x), float(y)) * texelSize;
            blur += texture(u_Original, uv) * w[x + 2] * w[y + 2];
        }
    }

    // 叠加混合: 原图 + Bloom ✕ 强度
    out_Color = vec4(original.rgb + blur.rgb * u_Intensity, original.a);
}
