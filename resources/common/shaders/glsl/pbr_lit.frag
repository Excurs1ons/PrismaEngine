// ============================================================
// pbr_lit.frag — PrismaEngine PBR 前向渲染片元着色器
//
// Metallic-Roughness 工作流:
//   - Cook-Torrance BRDF (GGX/Trowbridge-Reitz + Smith-GGX + Fresnel-Schlick)
//   - 多方向光 + 点光源循环 (PCF 阴影采样)
//   - IBL: 漫反射辐照度贴图 + 预过滤环境贴图 + BRDF LUT
//   - 法线贴图支持
//   - 级联阴影选择 (CSM)
//   - ACES 色调映射 + Gamma 校正
// ============================================================

#version 460 core

const float PBR_PI = 3.14159265359;
const float PBR_EPSILON = 0.0001;
const int MAX_LIGHTS = 32;
const int MAX_CASCADES = 4;

// ---- 片元输入 (来自 pbr_lit.vert) ----
layout(location = 0) in vec3 v_WorldPos;
layout(location = 1) in vec3 v_Normal;
layout(location = 2) in vec2 v_TexCoord;
layout(location = 3) in vec3 v_ViewDir;
layout(location = 4) in vec4 v_Color;

layout(location = 0) out vec4 outColor;

// ---- 光源结构 ----
struct PBRLight {
    vec4 position;  // xyz: position, w: padding
    vec4 color;     // rgb: color * intensity, w: range
    vec4 direction; // xyz: direction, w: type (0=dir, 1=point)
};

// ==================== Set 0: PBR 纹理 + 材质 ====================
// 布局必须与 Material::UpdateDescriptorSetPBR() 一致

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

// ==================== Set 2: 光源数据 ====================

layout(set = 2, binding = 0, std430) readonly buffer LightBuffer {
    PBRLight lights[];
} lightBuffer;

// ==================== Set 3: 阴影贴图 ====================

layout(set = 3, binding = 0) uniform sampler2DShadow ShadowMapArray[MAX_CASCADES];

// ==================== Set 4: IBL 环境光照 ====================

layout(set = 4, binding = 0) uniform samplerCube IrradianceMap;
layout(set = 4, binding = 1) uniform samplerCube PrefilterMap;
layout(set = 4, binding = 2) uniform sampler2D BRDFLUT;

// ===================== 法线贴图采样 =====================

vec3 SampleNormalMap(sampler2D normalMap, vec2 uv, mat3 TBN) {
    vec3 normal = texture(normalMap, uv).xyz;
    normal = normal * 2.0 - 1.0; // 从 [0,1] 映射到 [-1,1]
    normal = normalize(TBN * normal);
    return normal;
}

mat3 ComputeTBN(vec3 N, vec3 pos, vec2 uv) {
    // 使用偏导数计算世界空间的切线和副切线
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

// ===================== PBR 核心函数 =====================

// Fresnel-Schlick 近似
vec3 FresnelSchlick(vec3 f0, float cosTheta) {
    return f0 + (1.0 - f0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

// Fresnel-Schlick 带粗糙度 (用于 IBL)
vec3 FresnelSchlickRoughness(vec3 f0, float cosTheta, float roughness) {
    return f0 + (max(vec3(1.0 - roughness), f0) - f0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

// GGX 法线分布函数 (Trowbridge-Reitz)
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float alpha = max(roughness * roughness, PBR_EPSILON);
    float alpha2 = alpha * alpha;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = NdotH2 * (alpha2 - 1.0) + 1.0;
    denom = max(PBR_PI * denom * denom, PBR_EPSILON);

    return alpha2 / denom;
}

// Smith-Schlick 几何函数 (GGX)
float SchlickGGX(float NdotV, float roughness) {
    float r = max(roughness, PBR_EPSILON);
    float k = (r * r) / 2.0;
    float denom = NdotV * (1.0 - k) + k;
    return NdotV / max(denom, PBR_EPSILON);
}

// Smith 联合几何函数
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return SchlickGGX(NdotV, roughness) * SchlickGGX(NdotL, roughness);
}

// Cook-Torrance BRDF 镜面反射项
vec3 SpecularBRDF(vec3 N, vec3 V, vec3 L, vec3 H, float roughness, vec3 f0) {
    float D = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = FresnelSchlick(f0, max(dot(H, V), 0.0));
    vec3 num = D * G * F;
    float denom = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + PBR_EPSILON;
    return num / denom;
}

// ===================== 阴影采样 (PCF) =====================

float SampleShadowMap(sampler2DShadow shadowMap, vec3 projCoords, float bias) {
    // Percentage-Closer Filtering (PCF) 使用 4x4 泊松分布采样
    const vec2 poissonDisk[16] = vec2[](
        vec2(-0.942016, -0.399062), vec2(0.945586, -0.768907),
        vec2(-0.094185, -0.929058), vec2(0.344959,  0.293877),
        vec2(-0.915885,  0.457714), vec2(-0.815442, -0.879124),
        vec2(-0.382775,  0.276768), vec2(0.974843,  0.756483),
        vec2(0.443233, -0.975115), vec2(0.537429, -0.473734),
        vec2(-0.264969, -0.418930), vec2(0.791975,  0.190901),
        vec2(-0.241888,  0.997065), vec2(-0.814099,  0.914376),
        vec2(0.199841,  0.786413), vec2(0.143831, -0.141007)
    );

    float shadow = 0.0;
    int samples = 12;
    for (int i = 0; i < samples; i++) {
        vec2 offset = poissonDisk[i] * 0.002; // 采样半径
        shadow += texture(shadowMap, vec3(projCoords.xy + offset, projCoords.z - bias));
    }
    return shadow / float(samples);
}

// 级联阴影选择与采样
float CascadeShadow(vec3 worldPos, float cascadeCount) {
    // 简化实现: 使用距离选择级联
    const float cascadeDistances[4] = float[](25.0, 60.0, 150.0, 400.0);
    float dist = length(scene.cameraPos.xyz - worldPos);

    int cascadeIndex = 0;
    for (int i = 0; i < int(cascadeCount); i++) {
        if (dist > cascadeDistances[i]) {
            cascadeIndex = i + 1;
        }
    }
    cascadeIndex = min(cascadeIndex, int(cascadeCount) - 1);

    // 注意: 实际级联的 lightVP 矩阵由 C++ 在 ShadowPass 中通过 push constants 传递
    // 此处简化处理: 假设 ShadowMapArray 的每个级联已经过正确变换
    // 使用第一级联作为演示
    vec4 fragPosLightSpace = scene.viewProjection * vec4(worldPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0) return 1.0;

    return SampleShadowMap(ShadowMapArray[cascadeIndex], projCoords, 0.005);
}

// ===================== IBL 采样 =====================

vec3 SampleIBLDiffuse(vec3 N, vec3 V, vec3 F0, float roughness, float NdotV) {
    vec3 kS = FresnelSchlickRoughness(F0, NdotV, roughness);
    vec3 kD = (1.0 - kS) * (1.0 - material.metallic);

    // 漫反射辐照度
    vec3 irradiance = texture(IrradianceMap, N).rgb;
    vec3 diffuse = irradiance * material.baseColor.rgb;

    // 镜面反射 IBL: 预过滤环境贴图 + BRDF LUT
    vec3 prefilteredColor = textureLod(PrefilterMap, reflect(-V, N), roughness * 4.0).rgb;
    vec2 brdf = texture(BRDFLUT, vec2(NdotV, roughness)).rg;
    vec3 specular = prefilteredColor * (F0 * brdf.x + brdf.y);

    return kD * diffuse + specular;
}

// ===================== 对单个光源计算 PBR =====================

vec3 CalculatePBRLight(vec3 albedo, float metallic, float roughness,
                        vec3 N, vec3 V, vec3 L, vec3 radiance,
                        float shadowFactor) {
    vec3 H = normalize(V + L);

    // 漫反射 / 镜面反射能量分配
    vec3 f0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = FresnelSchlick(f0, max(dot(H, V), 0.0));

    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);

    float NdotL = max(dot(N, L), 0.0);

    vec3 diffuse = kD * albedo / PBR_PI;
    vec3 specular = SpecularBRDF(N, V, L, H, roughness, f0);

    return (diffuse + specular) * radiance * NdotL * shadowFactor;
}

// 获取光源方向
vec3 GetLightDir(PBRLight light, vec3 fragPos) {
    int type = int(light.direction.w + 0.5);
    if (type == 0) {
        return normalize(-light.direction.xyz); // 方向光
    } else {
        return normalize(light.position.xyz - fragPos); // 点光源
    }
}

// 获取光源辐射度
vec3 GetLightRadiance(PBRLight light, vec3 fragPos) {
    int type = int(light.direction.w + 0.5);
    vec3 radiance = light.color.rgb;

    if (type == 1) {
        // 点光源: 距离衰减
        vec3 toLight = light.position.xyz - fragPos;
        float dist = length(toLight);
        float range = light.color.w;
        if (range > 0.0 && dist < range) {
            float atten = max(1.0 - (dist * dist) / (range * range), 0.0);
            radiance *= atten * atten;
        }
    }

    return radiance;
}

// ===================== 色调映射 =====================

vec3 ACESToneMapping(vec3 color, float exposure) {
    color *= exposure;
    const float A = 2.51;
    const float B = 0.03;
    const float C = 2.43;
    const float D = 0.59;
    const float E = 0.14;
    return (color * (A * color + B)) / (color * (C * color + D) + E);
}

vec3 GammaCorrection(vec3 color, float gamma) {
    return pow(max(color, vec3(0.0)), vec3(1.0 / gamma));
}

// ===================== 主函数 =====================

void main() {
    // ===== 1. 采样材质贴图 =====
    vec4 albedoSample = texture(AlbedoMap, v_TexCoord);
    vec3 albedo = material.baseColor.rgb * albedoSample.rgb * v_Color.rgb;
    float alpha = albedoSample.a * material.baseColor.a;

    // Metallic-Roughness (GLTF 规范: B=metallic, G=roughness)
    float metallic = material.metallic;
    float roughness = material.roughness;
    vec4 mrSample = texture(MetallicRoughnessMap, v_TexCoord);
    metallic *= mrSample.b;
    roughness *= mrSample.g;

    // AO (环境光遮蔽)
    float ao = material.ao;
    ao *= texture(AOMap, v_TexCoord).r;

    // 自发光
    vec3 emissive = vec3(0.0);
    if (material.emissiveIntensity > PBR_EPSILON) {
        emissive = texture(EmissiveMap, v_TexCoord).rgb * material.emissiveIntensity;
    }

    // ===== 2. 法线计算 =====
    vec3 N = normalize(v_Normal);
    // 法线贴图: 若有则采样并转换到世界空间
    mat3 TBN = ComputeTBN(N, v_WorldPos, v_TexCoord);
    N = SampleNormalMap(NormalMap, v_TexCoord, TBN);

    // ===== 3. 直接光照 =====
    vec3 V = normalize(v_ViewDir);

    // 阴影因子 (基于世界位置)
    float shadowFactor = CascadeShadow(v_WorldPos, 4.0);

    vec3 directLighting = vec3(0.0);

    // 遍历所有光源
    for (int i = 0; i < MAX_LIGHTS; i++) {
        PBRLight light = lightBuffer.lights[i];
        vec3 L = GetLightDir(light, v_WorldPos);
        vec3 radiance = GetLightRadiance(light, v_WorldPos);

        // 跳过无效光源 (无辐射度)
        if (length(radiance) < PBR_EPSILON) continue;

        // 方向光应用阴影
        float lightShadow = 1.0;
        if (int(light.direction.w + 0.5) == 0) {
            lightShadow = shadowFactor;
        }

        directLighting += CalculatePBRLight(albedo, metallic, roughness, N, V, L, radiance, lightShadow);
    }

    // ===== 4. IBL 环境光照 =====
    float NdotV = max(dot(N, V), 0.0);
    vec3 f0 = mix(vec3(0.04), albedo, metallic);
    vec3 iblLighting = SampleIBLDiffuse(N, V, f0, roughness, NdotV) * ao;

    // ===== 5. 合成最终颜色 =====
    vec3 finalColor = directLighting + iblLighting + emissive;

    // ===== 6. 后处理 =====
    finalColor = ACESToneMapping(finalColor, 1.0);
    finalColor = GammaCorrection(finalColor, 2.2);

    outColor = vec4(finalColor, alpha);
}
