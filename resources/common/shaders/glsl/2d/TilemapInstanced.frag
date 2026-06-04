#version 460

// ============================================================================
// PrismaEngine — Tilemap Instanced Fragment Shader
// 从纹理图集采样，支持 alpha discard
// ============================================================================

// --- 输入从顶点着色器 ---
layout(location = 0) in vec2 v_TexCoord;

// --- 描述符资源 ---
// 纹理采样器 (binding = 0)
layout(binding = 0) uniform sampler2D u_Texture;

// --- 输出 ---
layout(location = 0) out vec4 out_Color;

void main() {
    out_Color = texture(u_Texture, v_TexCoord);

    // Alpha discard: 丢弃近乎透明的像素，避免透明 tile 遮挡
    if (out_Color.a < 0.01) {
        discard;
    }
}
