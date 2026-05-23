#version 450
layout(location = 0) in vec4 inPosition;

layout(push_constant) uniform PC {
    mat4 mvp;
    vec4 color;
} pc;

void main() {
    gl_Position = pc.mvp * vec4(inPosition.xyz, 1.0);
}
