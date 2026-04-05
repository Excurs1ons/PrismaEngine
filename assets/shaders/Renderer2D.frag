#version 450

layout(location = 0) in vec4 v_Color;
layout(location = 1) in vec2 v_UV;

layout(set = 0, binding = 0) uniform sampler2D u_AlbedoMap;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 texColor = texture(u_AlbedoMap, v_UV);
    outColor = v_Color * texColor;
    
    // 如果没有纹理（或者采样到全黑/全白），可能需要一个开关
    // 但在这里，我们假设没有纹理时会有一个默认的白色 1x1 纹理
}
