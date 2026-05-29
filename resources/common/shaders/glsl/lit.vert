// ============================================================
// lit.vert — PrismaEngine PBR 前向渲染顶点着色器
//
// 输入: position, color, uv, normal, texCoord, tangent (Vertex 结构体)
// 输出: 世界空间位置, 法线, UV, 视角方向, 顶点颜色
// Push Constants: 世界矩阵, 颜色
// Set 1, Binding 0: 场景数据 (视图, 投影, 相机位置)
// ============================================================

#version 450

// ---- 顶点输入 (匹配 Vertex 结构体布局) ----
layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec4 inUV;
layout(location = 3) in vec4 inNormal;
layout(location = 4) in vec4 inTexCoord;
layout(location = 5) in vec4 inTangent;

// ---- 片元着色器输出 ----
layout(location = 0) out vec3 v_WorldPos;
layout(location = 1) out vec3 v_Normal;
layout(location = 2) out vec2 v_UV;
layout(location = 3) out vec3 v_ViewDir;
layout(location = 4) out vec4 v_Color;

// ---- Push Constants (每物体) ----
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
    vec4 worldPos = pc.world * vec4(inPosition.xyz, 1.0);
    gl_Position = scene.viewProjection * worldPos;

    // 输出世界空间位置
    v_WorldPos = worldPos.xyz;

    // 世界空间法线 (使用逆转置矩阵)
    mat3 normalMatrix = mat3(transpose(inverse(pc.world)));
    v_Normal = normalize(normalMatrix * inNormal.xyz);

    // UV 坐标 (使用 inUV.xy, 若无效则回退到 inTexCoord.xy)
    v_UV = inUV.xy;

    // 视角方向 (从片元指向相机)
    v_ViewDir = scene.cameraPos.xyz - worldPos.xyz;

    // 传递顶点颜色
    v_Color = inColor * pc.color;
}
