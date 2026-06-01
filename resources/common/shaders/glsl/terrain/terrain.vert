#version 460
// ============================================================================
// PrismaEngine — Terrain Vertex Shader
// PBR 地形 + 4 层纹理喷溅
// 输入: position(float3), normal(float3), uv(float2)
// 输出: worldPos, normal, uv, clipPos
// Set 0, Binding 0: TerrainUniforms (viewProj, cameraPos, layer params)
// ============================================================================

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;

// 输出到片元
layout(location = 0) out vec3 v_WorldPos;
layout(location = 1) out vec3 v_Normal;
layout(location = 2) out vec2 v_TexCoord;
layout(location = 3) out float v_Height;

// 地形 Uniform 缓冲 (std140)
layout(std140, binding = 0) uniform TerrainUniforms {
    mat4  u_ViewProjection;
    vec4  u_CameraPos;        // xyz = position, w = padding
    vec4  u_BlendHeights;     // 4 层混合中心高度
    vec4  u_BlendWidths;      // 4 层混合过渡宽度
    vec4  u_SlopeFactors;     // 4 层坡度影响系数
    vec4  u_Tiling;           // x=tiling0-1, y=tiling2-3, zw=unused
    vec4  u_HeightRange;      // x=minHeight, y=maxHeight, z=invRange, w=unused
    vec4  u_PBRParams;        // x=roughness, y=metallic, z=AO, w=unused
    vec4  u_SunDirection;     // xyz=direction, w=unused
    vec4  u_SunColor;         // rgb=color*intensity, w=ambient intensity
} terrain;

void main() {
    // 世界空间位置（地形模型矩阵为单位矩阵）
    vec3 worldPos = a_Position;
    gl_Position = terrain.u_ViewProjection * vec4(worldPos, 1.0);

    // 输出到片元
    v_WorldPos = worldPos;
    v_Normal = normalize(a_Normal);
    v_TexCoord = a_TexCoord;
    v_Height = a_Position.y;
}
