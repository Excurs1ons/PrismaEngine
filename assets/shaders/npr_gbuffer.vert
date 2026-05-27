// ============================================================
// npr_gbuffer.vert - PrismaEngine NPR 延迟渲染几何通道顶点着色器
//
// 渲染场景几何体到 NPR G-Buffer
// Push Constants: 世界矩阵, 颜色
// UBO Set 1: 视图矩阵, 投影矩阵, 视口投影矩阵, 相机位置
// ============================================================

#version 450

// ---- 顶点输入 ----
layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec4 inUV;
layout(location = 3) in vec4 inNormal;
layout(location = 4) in vec4 inTexCoord;
layout(location = 5) in vec4 inTangent;

// ---- 片元着色器输出 ----
layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outUV;
layout(location = 3) out vec4 outColor;
layout(location = 4) out vec3 outTangent;

// ---- Push Constants (每物体) ----
layout(push_constant) uniform PushConstants {
    mat4 world;
    vec4 color;
} pc;

// ---- Set 1: 场景数据 (相机) ----
layout(set = 1, binding = 0) uniform SceneData {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec4 cameraPos;
} scene;

void main() {
    mat4 worldMatrix = pc.world;
    vec4 worldPos = worldMatrix * vec4(inPosition.xyz, 1.0);
    gl_Position = scene.viewProjection * worldPos;

    // 世界空间位置
    outWorldPos = worldPos.xyz;

    // 世界空间法线 (使用逆转置矩阵)
    mat3 normalMatrix = mat3(transpose(inverse(worldMatrix)));
    outNormal = normalize(normalMatrix * inNormal.xyz);

    // UV 坐标
    outUV = inUV.xy;

    // 顶点颜色
    outColor = inColor * pc.color;

    // 世界空间切线 (如果存在)
    if (length(inTangent.xyz) > 0.001) {
        outTangent = normalize(normalMatrix * inTangent.xyz);
    } else {
        outTangent = vec3(0.0);
    }
}
