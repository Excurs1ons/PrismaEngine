#version 460
// ============================================================================
// PrismaEngine — Grayscale 后处理片段着色器
// 将渲染结果转换为灰度图像。
//
// 配合 FullscreenTri.vert 使用
// (v_TexCoord 由顶点着色器通过 gl_VertexIndex 生成)
// ============================================================================

layout(location = 0) in vec2 v_TexCoord;
layout(location = 0) out vec4 out_Color;

layout(binding = 0) uniform sampler2D u_Texture;

layout(push_constant) uniform GrayscaleParams {
    float u_Intensity; // 灰度强度 (0.0 = 原图, 1.0 = 完全灰度)
};

void main() {
    vec4 color = texture(u_Texture, v_TexCoord);

    // 标准亮度公式: 0.299 R + 0.587 G + 0.114 B
    float gray = dot(color.rgb, vec3(0.299, 0.587, 0.114));

    // 在原图和灰度之间插值
    vec3 result = mix(color.rgb, vec3(gray), u_Intensity);

    out_Color = vec4(result, color.a);
}
