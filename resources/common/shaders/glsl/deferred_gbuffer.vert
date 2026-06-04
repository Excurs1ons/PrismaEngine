#version 460 core
// ============================================================
// deferred_gbuffer.vert — PrismaEngine 延迟渲染几何通道顶点着色器
//
// 全屏三角形 VS，无需 MVP 变换。
// 输出 UV 和视空间位置到片元着色器。
// ============================================================

// ---- 片元输出 ----
layout(location = 0) out vec2 v_TexCoord;
layout(location = 1) out vec3 v_ViewPos;

void main() {
    // 全屏三角形: 三个顶点覆盖整个裁剪空间
    // 顶点 ID 0,1,2 映射到 (-1,-1), (3,-1), (-1,3)
    float x = float(gl_VertexIndex == 1 ? 3 : -1);
    float y = float(gl_VertexIndex == 2 ? 3 : -1);
    gl_Position = vec4(x, y, 0.0, 1.0);

    // UV: 从裁剪空间 [-1,1] 映射到纹理空间 [0,1]
    v_TexCoord = gl_Position.xy * 0.5 + 0.5;

    // 视空间位置 (用于重建世界位置等)
    v_ViewPos = vec3(gl_Position.xy, 0.0);
}
