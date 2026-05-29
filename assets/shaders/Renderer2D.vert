#version 450

// Vertex2D: posUV.xy = position, posUV.zw = uv
layout(location = 0) in vec4 inPosUV;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 v_Color;
layout(location = 1) out vec2 v_UV;

layout(push_constant) uniform PushConstants {
    mat4 mvp;
} pc;

void main() {
    gl_Position = pc.mvp * vec4(inPosUV.xy, 0.0, 1.0);
    v_Color = inColor;
    v_UV = inPosUV.zw;
}
