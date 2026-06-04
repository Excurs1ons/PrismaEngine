#version 460
#extension GL_EXT_scalar_block_layout : require

// ============================================================================
// PrismaEngine — 粒子渲染顶点着色器
// 面向相机的公告板（billboard），生成四边形
// ============================================================================

// 粒子数据 SSBO（与计算着色器共享布局）
struct ParticleData {
    vec4 position;   // xyz=pos, w=size
    vec4 velocity;   // xyz=vel, w=rotation
    vec4 color;      // rgba
    vec4 life;       // x=age, y=lifetime, z=unused, w=unused (w < 0 = dead)
};

layout(std430, binding = 0) buffer ParticleBuffer {
    ParticleData particles[];
};

// 相机 Uniform
layout(std140, binding = 1) uniform CameraUniforms {
    mat4  u_ViewProjection;
    vec3  u_CameraRight;
    vec3  u_CameraUp;
    float u_Near;
    vec3  u_CameraForward;
    float u_Far;
    float u_Use2D;   // 1.0 = 2D mode, 0.0 = 3D mode
} camera;

// 输出到片段着色器
layout(location = 0) out vec2 v_TexCoord;
layout(location = 1) out vec4 v_Color;
layout(location = 2) flat out float v_Lifetime;
layout(location = 3) flat out float v_Age;

// 用于生成公告板四边形的 4 个角偏移
// 0: (-1, -1), 1: (1, -1), 2: (-1, 1), 3: (1, 1)
const vec2 cornerOffsets[4] = vec2[](
    vec2(-0.5, -0.5),
    vec2( 0.5, -0.5),
    vec2(-0.5,  0.5),
    vec2( 0.5,  0.5)
);

const vec2 texCoords[4] = vec2[](
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0)
);

void main() {
    // gl_VertexID / 4 = 粒子索引
    // gl_VertexID % 4 = 四边形顶点索引
    uint particleIdx = gl_VertexIndex / 4;
    uint cornerIdx   = gl_VertexIndex % 4;

    ParticleData p = particles[particleIdx];

    // 跳过死亡粒子（将顶点放到裁剪空间外）
    if (p.life.w < 0.0 || p.life.x >= p.life.y) {
        gl_Position = vec4(0.0, 0.0, 0.0, 0.0);
        return;
    }

    // 相机面向公告板（2D 模式使用固定朝向）
    vec2 offset = cornerOffsets[cornerIdx];
    vec3 right = (camera.u_Use2D > 0.5) ? vec3(1.0, 0.0, 0.0) : camera.u_CameraRight;
    vec3 up    = (camera.u_Use2D > 0.5) ? vec3(0.0, 1.0, 0.0) : camera.u_CameraUp;
    vec3 quadPos = p.position.xyz
                 + right * offset.x * p.position.w
                 + up    * offset.y * p.position.w;

    gl_Position = camera.u_ViewProjection * vec4(quadPos, 1.0);

    v_TexCoord = texCoords[cornerIdx];
    v_Color    = p.color;
    v_Lifetime = p.life.y;
    v_Age      = p.life.x;
}
