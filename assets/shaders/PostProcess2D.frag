#version 450

// PostProcess2D — 基础 pass-through 片段着色器
// 用于 PostProcessPass2D 的默认/基础 PSO
// 将输入纹理直接输出，不做任何后处理效果

layout(location = 0) in vec2 v_TexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D u_Sampler;

void main() {
    outColor = texture(u_Sampler, v_TexCoord);
}