// ============================================================
// pbr_gbuffer.vert - PrismaEngine PBR 延迟渲染几何通道顶点着色器
//
// 渲染场景几何体到 G-Buffer 的多渲染目标
// Push Constants: VP, 世界矩阵, 颜色, 自发光
// ============================================================

#version 450

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec4 inUV;
layout(location = 3) in vec4 inNormal;
layout(location = 4) in vec4 inTexCoord;
layout(location = 5) in vec4 inTangent;

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outUV;
layout(location = 3) out vec4 outColor;
layout(location = 4) out vec3 outTangent;

layout(push_constant) uniform PushConstants {
    mat4 vp;
    mat4 model;
    vec4 color;
    vec4 emissive;
    vec4 materialParams; // x=metallic, y=roughness, z=ao, w=emissiveIntensity
} pc;

void main() {
    vec4 worldPos = pc.model * vec4(inPosition.xyz, 1.0);
    gl_Position = pc.vp * worldPos;

    outWorldPos = worldPos.xyz;

    // 世界空间法线
    mat3 normalMatrix = mat3(transpose(inverse(pc.model)));
    outNormal = normalize(normalMatrix * inNormal.xyz);

    outUV = inUV.xy;
    outColor = inColor * pc.color;

    // 世界空间切线
    if (length(inTangent.xyz) > 0.001) {
        outTangent = normalize(normalMatrix * inTangent.xyz);
    } else {
        outTangent = vec3(0.0);
    }
}
