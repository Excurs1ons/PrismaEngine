#version 460 core
// ============================================================
// deferred_gbuffer.frag — PrismaEngine 延迟渲染几何通道片元着色器
//
// 从场景渲染结果填充 G-buffer (MRT):
//   RT0: RGB 世界位置 + A 粗糙度 (R16G16B16A16_SFLOAT)
//   RT1: RGB 法线 + A 金属度 (R16G16B16A16_SFLOAT)
//   RT2: RGB 反照率 + A 环境光遮蔽 (R8G8B8A8_UNORM)
//   RT3: RGB 自发光 + A 材质 ID (R16G16B16A16_SFLOAT)
//   深度: D32_SFLOAT
// ============================================================

const float PBR_EPSILON = 0.0001;

// ---- 片元输入 (来自 deferred_gbuffer.vert) ----
layout(location = 0) in vec2 v_TexCoord;
layout(location = 1) in vec3 v_ViewPos;

// ---- MRT 输出 (匹配 GBufferTarget 枚举) ----
layout(location = 0) out vec4 outPosition;   // GBufferTarget::Position
layout(location = 1) out vec4 outNormal;     // GBufferTarget::Normal
layout(location = 2) out vec4 outAlbedo;     // GBufferTarget::Albedo
layout(location = 3) out vec4 outEmissive;   // GBufferTarget::Emissive

// ==================== Set 0: 材质数据 ====================

layout(set = 0, binding = 0, std140) uniform MaterialData {
    vec4 baseColor;
    float metallic;
    float roughness;
    float ao;
    float emissiveIntensity;
} material;

layout(set = 0, binding = 1) uniform sampler2D AlbedoMap;
layout(set = 0, binding = 2) uniform sampler2D NormalMap;
layout(set = 0, binding = 3) uniform sampler2D MetallicRoughnessMap;
layout(set = 0, binding = 4) uniform sampler2D AOMap;
layout(set = 0, binding = 5) uniform sampler2D EmissiveMap;

// ==================== Set 1: 场景数据 ====================

layout(set = 1, binding = 0, std140) uniform SceneData {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec4 cameraPos;
} scene;

// ==================== 顶点属性 (延迟渲染用 G-Pass 顶点) ====================

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aTangent;

// ==================== Push Constants ====================

layout(push_constant) uniform PushConstants {
    mat4 world;
    vec4 color;
} pc;

// ===================== 法线贴图采样 =====================

vec3 SampleNormalMap(sampler2D normalMap, vec2 uv, mat3 TBN) {
    vec3 normal = texture(normalMap, uv).xyz;
    normal = normal * 2.0 - 1.0;
    return normalize(TBN * normal);
}

mat3 ComputeTBN(vec3 N, vec3 pos, vec2 uv) {
    vec3 ddxPos = dFdx(pos);
    vec3 ddyPos = dFdy(pos);
    vec2 ddxUV = dFdx(uv);
    vec2 ddyUV = dFdy(uv);

    vec3 T = ddxPos * ddyUV.t - ddyPos * ddxUV.t;
    vec3 B = ddyPos * ddxUV.s - ddxPos * ddyUV.s;

    float lenT = length(T);
    float lenB = length(B);
    if (lenT > PBR_EPSILON) T /= lenT;
    if (lenB > PBR_EPSILON) B /= lenB;

    return mat3(T, B, N);
}

void main() {
    // ===== 1. 采样材质贴图 =====
    vec4 albedoSample = texture(AlbedoMap, aTexCoord);
    vec3 albedo = material.baseColor.rgb * albedoSample.rgb;

    float metallic = material.metallic;
    float roughness = material.roughness;
    vec4 mrSample = texture(MetallicRoughnessMap, aTexCoord);
    metallic *= mrSample.b;
    roughness *= mrSample.g;

    float ao = material.ao;
    ao *= texture(AOMap, aTexCoord).r;

    vec3 emissive = vec3(0.0);
    if (material.emissiveIntensity > PBR_EPSILON) {
        emissive = texture(EmissiveMap, aTexCoord).rgb * material.emissiveIntensity;
    }

    // ===== 2. 世界空间位置 =====
    vec4 worldPos = pc.world * vec4(aPos, 1.0);
    vec3 worldPosOut = worldPos.xyz;

    // ===== 3. 法线计算 =====
    mat3 normalMatrix = transpose(inverse(mat3(pc.world)));
    vec3 N = normalize(normalMatrix * aNormal);

    // 法线贴图: 若有则采样
    mat3 TBN = ComputeTBN(N, worldPosOut, aTexCoord);
    N = SampleNormalMap(NormalMap, aTexCoord, TBN);

    // ===== 4. 写入 G-buffer =====
    outPosition = vec4(worldPosOut, roughness);
    outNormal   = vec4(N * 0.5 + 0.5, metallic); // 法线编码到 [0,1]
    outAlbedo   = vec4(albedo, ao);
    outEmissive = vec4(emissive, 0.0); // 材质 ID = 0
}
