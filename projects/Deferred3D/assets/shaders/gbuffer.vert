#version 450
layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec4 inUV;

layout(push_constant) uniform PushConstants {
    mat4 vp;
    mat4 model;
    vec4 color;
} pc;

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outUV;

void main() {
    vec4 worldPos = pc.model * vec4(inPosition.xyz, 1.0);
    outWorldPos = worldPos.xyz;
    outNormal = vec3(0, 1, 0);
    outUV = inUV.xy;
    gl_Position = pc.vp * worldPos;
}
