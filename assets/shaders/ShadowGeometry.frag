#version 450
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform ShadowPC {
    mat4 u_MVP;
    float u_ShadowIntensity;
    float padding[3]; // C++ alignas(16) 使 sizeof = 80
} pc;

void main() {
    outColor = vec4(0.0, 0.0, 0.0, pc.u_ShadowIntensity);
}
