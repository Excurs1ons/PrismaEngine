#version 460
// ============================================================================
// PrismaEngine — 扭曲效果片段着色器
// 基于时间的正弦波 UV 偏移，产生扭曲/波纹效果。
//
// 配合 FullscreenTri.vert 使用
// (v_TexCoord 由顶点着色器通过 gl_VertexIndex 生成)
// ============================================================================

layout(location = 0) in vec2 v_TexCoord;
layout(location = 0) out vec4 out_Color;

layout(binding = 0) uniform sampler2D u_Texture;

layout(push_constant) uniform DistortionParams {
    float u_Time;    // 累计时间
    float u_Speed;   // 扭曲速度
    float u_Amount;  // 扭曲幅度
    float u_Warp;    // 扭曲频率/波纹密度
};

void main() {
    vec2 uv = v_TexCoord;

    // 基于时间的正弦波 UV 偏移
    // 水平偏移: sin(time * speed + uv.y * warp) * amount
    uv.x += sin(u_Time * u_Speed + uv.y * u_Warp) * u_Amount;
    // 垂直偏移: cos(time * speed + uv.x * warp) * amount
    uv.y += cos(u_Time * u_Speed + uv.x * u_Warp) * u_Amount;

    out_Color = texture(u_Texture, uv);
}
