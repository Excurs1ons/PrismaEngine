#version 450

layout(location = 0) in vec4 a_Position;
layout(location = 1) in vec4 a_Color;

layout(location = 0) out vec4 v_Color;

layout(push_constant) uniform QuadPushConstants {
    mat4 u_MVP;
    vec4 u_Color;
} pc;

void main() {
    vec4 pos = pc.u_MVP * a_Position;
    gl_Position = vec4(pos.x, -pos.y, pos.z, pos.w);
    v_Color = a_Color * pc.u_Color;
}
