// ============================================================
// pbr_lit.vert — PrismaEngine PBR 延迟/前向着色顶点着色器
//
// 输入: 位置, 法线, 纹理坐标, 顶点颜色
// 输出: 世界空间位置, 法线, UV, 视角方向, 顶点颜色
// Push Constants: 世界矩阵, 颜色
// Set 1, Binding 0: 场景数据 (视图投影, 相机位置)
// ============================================================

#version 460 core

// ---- 顶点输入 ----
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec4 aColor;

// ---- 片元输出 ----
layout(location = 0) out vec3 v_WorldPos;
layout(location = 1) out vec3 v_Normal;
layout(location = 2) out vec2 v_TexCoord;
layout(location = 3) out vec3 v_ViewDir;
layout(location = 4) out vec4 v_Color;

// ---- Push Constants (匹配 C++ PBRPushConstants) ----
layout(push_constant) uniform PushConstants {
    mat4 world;
    vec4 color;
} pc;

// ---- Set 1: 场景数据 (相机) ----
layout(set = 1, binding = 0, std140) uniform SceneData {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec4 cameraPos;
} scene;

void main() {
    // 世界空间位置
    vec4 worldPos = pc.world * vec4(aPos, 1.0);
    gl_Position = scene.viewProjection * worldPos;

    // 输出世界空间位置
    v_WorldPos = worldPos.xyz;

    // 世界空间法线 (使用逆转置矩阵)
    mat3 normalMatrix = transpose(inverse(mat3(pc.world)));
    v_Normal = normalize(normalMatrix * aNormal);

    // UV 坐标
    v_TexCoord = aTexCoord;

    // 视角方向 (从片元指向相机)
    v_ViewDir = scene.cameraPos.xyz - worldPos.xyz;

    // 传递顶点颜色
    v_Color = aColor * pc.color;
}
