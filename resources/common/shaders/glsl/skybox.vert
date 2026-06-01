#version 460 core
// ============================================================
// skybox.vert — PrismaEngine 天空盒顶点着色器
//
// 渲染无限远的天空盒:
//   - 移除视图矩阵的平移分量, 使天空盒始终跟随相机
//   - 将深度设为最大 (z=w) 以确保天空盒在最远平面
//   - 输入: 位置 (立方体顶点)
// ============================================================

// ---- 顶点输入 ----
layout(location = 0) in vec3 aPos;

// ---- 片元输出 ----
layout(location = 0) out vec3 v_LocalPos;

// ---- Push Constants ----
layout(push_constant) uniform SkyboxPushConstants {
    mat4 viewProjection; // 已移除平移的 VP 矩阵
} pc;

void main() {
    // 使用局部位置作为立方体贴图的方向向量
    v_LocalPos = aPos;

    // 变换到裁剪空间 (VP 矩阵已移除平移)
    vec4 clipPos = pc.viewProjection * vec4(aPos, 1.0);

    // 设置深度为最大 (z=w), 确保天空盒在最远平面
    gl_Position = clipPos.xyww;
}
