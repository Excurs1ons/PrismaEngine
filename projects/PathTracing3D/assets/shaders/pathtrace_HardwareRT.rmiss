#version 460
#extension GL_EXT_ray_tracing : require

struct HitPayload {
    vec3 color;
    vec3 normal;
    float emissive;
    int objIndex;
    float t;
    int hitType;
};

layout(location = 4) rayPayloadInEXT HitPayload prd;

void main() {
    prd.hitType = 0; // 未命中
    prd.objIndex = -1;
    prd.t = 1e30;
}
