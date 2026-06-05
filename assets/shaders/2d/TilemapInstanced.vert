#version 460

// ============================================================================
// PrismaEngine — Tilemap Instanced Vertex Shader
// 接收 per-vertex 位置/UV 和 per-instance 世界矩阵/UV 边界
// ============================================================================

// --- Per-vertex 输入 (slot 0, VERTEX rate) ---
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

// --- Per-instance 输入 (slot 1, INSTANCE rate) ---
// mat4 占用 locations 2,3,4,5 (每行一个 vec4)
layout(location = 2) in mat4 a_World;
layout(location = 6) in vec4 a_UV;    // x=u0, y=v0, z=u1, w=v1

// --- 描述符资源 ---
// Camera UBO (binding = 1)
layout(std140, binding = 1) uniform CameraUBO {
    mat4 u_ViewProjection;
};

// --- 输出到片段着色器 ---
layout(location = 0) out vec2 v_TexCoord;

void main() {
    // 将顶点 UV 从 [0,1] 映射到图集中的 UV 包围盒
    v_TexCoord = vec2(
        mix(a_UV.x, a_UV.z, a_TexCoord.x),
        mix(a_UV.y, a_UV.w, a_TexCoord.y)
    );

    // 世界空间 → 裁剪空间
    gl_Position = u_ViewProjection * a_World * vec4(a_Position, 1.0);
}
