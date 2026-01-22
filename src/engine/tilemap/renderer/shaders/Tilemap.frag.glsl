#version 450

// 来自顶点着色器的输入
layout(location = 0) in vec2 v_TexCoord;
layout(location = 1) in float v_TexIndex;
layout(location = 2) in vec4 v_Color;

// 纹理数组
layout(binding = 1) uniform sampler2D u_TilesetTextures[16];

// 输出颜色
layout(location = 0) out vec4 out_Color;

void main() {
    int texIndex = int(v_TexIndex);

    // 防止越界访问
    if (texIndex < 0 || texIndex >= 16) {
        out_Color = v_Color;
        return;
    }

    // 从对应纹理采样
    vec4 texColor = texture(u_TilesetTextures[texIndex], v_TexCoord);

    // 应用颜色乘法 (用于淡入淡出、着色等)
    out_Color = texColor * v_Color;

    // Alpha 测试 (可选)
    if (out_Color.a < 0.01) {
        discard;
    }
}
