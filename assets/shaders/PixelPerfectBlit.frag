#version 450

layout(location = 0) in vec2 v_TexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D u_Sampler;

void main() {
    // 最近邻采样：point sampler 确保像素清晰无模糊
    outColor = texture(u_Sampler, v_TexCoord);
}
