#version 450
layout(location = 0) in vec2 a_Position;

layout(push_constant) uniform ShadowPC {
    mat4 u_MVP;
    float u_ShadowIntensity;
    float padding[3]; // C++ alignas(16) 使 sizeof = 80
} pc;

void main() {
    gl_Position = pc.u_MVP * vec4(a_Position, 0.0, 1.0);
}
