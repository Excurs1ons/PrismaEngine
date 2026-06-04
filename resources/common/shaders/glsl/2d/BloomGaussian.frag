#version 460
// ============================================================================
// PrismaEngine — Bloom 高斯模糊片段着色器
// 5-tap 可分离高斯模糊，支持水平和垂直方向。
//
// 配合 FullscreenTri.vert 使用
// (v_TexCoord 由顶点着色器通过 gl_VertexIndex 生成)
// ============================================================================

layout(location = 0) in vec2 v_TexCoord;
layout(location = 0) out vec4 out_Color;

layout(binding = 0) uniform sampler2D u_Texture;

layout(push_constant) uniform BloomGaussianParams {
    vec2 u_Direction; // 模糊方向 (1,0) = 水平, (0,1) = 垂直
};

void main() {
    vec2 texelSize = 1.0 / textureSize(u_Texture, 0);
    vec2 offset = u_Direction * texelSize;

    // 5-tap Gaussian 权重 (Pascal 三角形第 5 行归一化: 1,4,6,4,1)
    const float weights[5] = float[](
        0.0625,  // 1/16
        0.25,    // 4/16
        0.375,   // 6/16
        0.25,    // 4/16
        0.0625   // 1/16
    );

    const vec2 offsets[5] = vec2[](
        vec2(-2.0, -2.0),
        vec2(-1.0, -1.0),
        vec2( 0.0,  0.0),
        vec2( 1.0,  1.0),
        vec2( 2.0,  2.0)
    );

    vec4 result = vec4(0.0);
    for (int i = 0; i < 5; i++) {
        vec2 uv = v_TexCoord + offsets[i] * offset;
        result += texture(u_Texture, uv) * weights[i];
    }

    out_Color = result;
}
