#version 460 core
// ============================================================
// deferred_fullscreen.vert — PrismaEngine 全屏三角形顶点着色器
//
// 用于光照合成和后期处理的通用全屏三角形 VS。
// 无需顶点缓冲，基于 gl_VertexIndex 生成。
// ============================================================

// ---- 片元输出 ----
layout(location = 0) out vec2 v_TexCoord;

void main() {
    // 全屏三角形: 覆盖裁剪空间
    float x = float(gl_VertexIndex == 1 ? 3 : -1);
    float y = float(gl_VertexIndex == 2 ? 3 : -1);
    gl_Position = vec4(x, y, 0.0, 1.0);

    // UV 坐标
    v_TexCoord = gl_Position.xy * 0.5 + 0.5;
}
