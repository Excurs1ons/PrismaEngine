// ============================================================
// pbr_gbuffer.frag - PrismaEngine PBR G-Buffer 片元着色器
//
// 输出 4 个渲染目标:
//   RT0: 世界位置 (RGB) + 粗糙度 (A)      - RGBA16F
//   RT1: 世界法线 (RGB) + 金属度 (A)       - RGBA16F
//   RT2: 反照率 (RGB) + AO (A)            - RGBA8_UNorm
//   RT3: 自发光 (RGB) + 材质标志 (A)        - RGBA16F
//
// 支持贴图: albedoMap, normalMap, metallicRoughnessMap, aoMap, emissiveMap
// ============================================================

#version 450

// ---- 片元输入 ----
layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec4 inColor;
layout(location = 4) in vec3 inTangent;

// ---- G-Buffer 多渲染目标输出 ----
layout(location = 0) out vec4 outPosition;   // RGB: 世界位置, A: 粗糙度
layout(location = 1) out vec4 outNormal;     // RGB: 世界法线, A: 金属度
layout(location = 2) out vec4 outAlbedo;     // RGB: 反照率,  A: AO
layout(location = 3) out vec4 outEmissive;   // RGB: 自发光,  A: 材质标志

// ---- Push Constants ----
layout(push_constant) uniform PushConstants {
    mat4 vp;
    mat4 model;
    vec4 color;
    vec4 emissive;
    vec4 materialParams; // x=metallic, y=roughness, z=ao, w=emissiveIntensity
} pc;

// ---- 材质贴图 (optional, set 0) ----
layout(set = 0, binding = 0) uniform sampler2D albedoMap;
layout(set = 0, binding = 1) uniform sampler2D normalMap;
layout(set = 0, binding = 2) uniform sampler2D metallicRoughnessMap;
layout(set = 0, binding = 3) uniform sampler2D aoMap;
layout(set = 0, binding = 4) uniform sampler2D emissiveMap;

// ---- 材质 Uniform ----
layout(set = 0, binding = 5, std140) uniform MaterialParams {
    vec4 baseColor;
    float metallic;
    float roughness;
    float ao;
    float emissiveIntensity;
} material;

void main() {
    // ===== 1. 反照率 =====
    vec4 albedoSample = texture(albedoMap, inUV);
    vec3 baseColor = inColor.rgb * pc.color.rgb * material.baseColor.rgb * albedoSample.rgb;

    // ===== 2. 金属度和粗糙度 =====
    float metallic = pc.materialParams.x * material.metallic;
    float roughness = pc.materialParams.y * material.roughness;
    if (textureSize(metallicRoughnessMap, 0).x > 1) {
        vec4 mrSample = texture(metallicRoughnessMap, inUV);
        metallic *= mrSample.b;  // GLTF: B = metallic
        roughness *= mrSample.g; // GLTF: G = roughness
    }

    // ===== 3. AO =====
    float ao = pc.materialParams.z * material.ao;
    if (textureSize(aoMap, 0).x > 1) {
        ao *= texture(aoMap, inUV).r;
    }

    // ===== 4. 自发光 =====
    float emissiveIntensity = pc.materialParams.w * material.emissiveIntensity;
    vec3 emissiveColor = pc.emissive.rgb * emissiveIntensity;
    if (emissiveIntensity > 0.001 && textureSize(emissiveMap, 0).x > 1) {
        emissiveColor += texture(emissiveMap, inUV).rgb * emissiveIntensity;
    }

    // ===== 5. 法线贴图 (如果存在切线) =====
    vec3 N = normalize(inNormal);
    if (length(inTangent) > 0.001 && textureSize(normalMap, 0).x > 1) {
        vec3 T = normalize(inTangent);
        vec3 B = cross(N, T);
        mat3 TBN = mat3(T, B, N);
        vec3 normalTS = texture(normalMap, inUV).rgb * 2.0 - 1.0;
        N = normalize(TBN * normalTS);
    }

    // ===== 输出 G-Buffer =====
    outPosition = vec4(inWorldPos, roughness);
    outNormal = vec4(N * 0.5 + 0.5, metallic);  // 法线编码到 [0,1]
    outAlbedo = vec4(baseColor, ao);
    outEmissive = vec4(emissiveColor, 1.0);
}
