#version 450
layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outAlbedo;
layout(location = 3) out vec4 outEmissive;

layout(push_constant) uniform PushConstants {
    mat4 vp;
    mat4 model;
    vec4 color;
} pc;

void main() {
    outPosition = vec4(inWorldPos, 1.0);
    vec3 N = normalize(cross(dFdx(inWorldPos), dFdy(inWorldPos)));
    outNormal = vec4(N, 0.0);
    outAlbedo = pc.color;
    outEmissive = vec4(0.0, 0.0, 0.0, 0.0);
}
