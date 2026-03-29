#version 460

// ========== 从顶点着色器输入 ==========

layout(location = 0) in vec2 v_TexCoord;     // 纹理坐标
layout(location = 1) in vec4 v_Color;         // 顶点颜色

// ========== Uniform 缓冲区 ==========

// 精灵 Uniform 缓冲区
layout(binding = 1) uniform SpriteUBO {
    vec4 u_Color;                // 颜色调制
    float u_Opacity;              // 透明度
    bool u_FlipX;                // X 轴翻转
    bool u_FlipY;                // Y 轴翻转
    bool u_UseColor;              // 是否使用颜色调制
};

// 纹理采样器
layout(binding = 2) uniform sampler2D u_Texture;

// ========== 输出 ==========

layout(location = 0) out vec4 out_Color;       // 最终颜色
layout(location = 1) out vec4 out_Position;    // 世界坐标（用于光照）

// ========== 主函数 ==========

void main() {
    // 应用纹理坐标翻转
    vec2 texCoord = v_TexCoord;
    if (u_FlipX) {
        texCoord.x = 1.0 - texCoord.x;
    }
    if (u_FlipY) {
        texCoord.y = 1.0 - texCoord.y;
    }

    // 采样纹理
    vec4 texColor = texture(u_Texture, texCoord);

    // 应用颜色调制
    if (u_UseColor) {
        out_Color = texColor * v_Color * u_Color;
    } else {
        out_Color = texColor * v_Color;
    }

    // 应用透明度
    out_Color.a *= u_Opacity;

    // Alpha 测试
    if (out_Color.a < 0.01) {
        discard;
    }

    // 输出世界坐标（简化，实际应该从顶点着色器传递）
    out_Position = vec4(texCoord, 0.0, 1.0);
}
