// ============================================================
// lit.frag — PrismaEngine PBR 前向渲染片元着色器
//
// Metallic-Roughness 工作流:
//   - Cook-Torrance BRDF (GGX 法线分布 + Smith 几何 + Fresnel-Schlick)
//   - 1 个方向光 + 最多 4 个点光源
//   - 纹理采样 (albedo, normal, metallic-roughness, ao, emissive)
//   - ACES 色调映射 + Gamma 校正
//
// 注意: 不包含 IBL (Image Based Lighting)
// ============================================================

#version 450

const float PBR_PI = 3.14159265359;
const float PBR_EPSILON = 0.0001;
const int MAX_LIGHTS = 32;

// ---- 片元输入 (来自 lit.vert) ----
layout(location = 0) in vec3 v_WorldPos;
layout(location = 1) in vec3 v_Normal;
layout(location = 2) in vec2 v_UV;
layout(location = 3) in vec3 v_ViewDir;
layout(location = 4) in vec4 v_Color;

layout(location = 0) out vec4 outColor;

// ---- 光源结构 (匹配 C++ Light struct in RenderTypes.h) ----
struct PBRLight {
    vec4 position;  // xyz: position, w: padding
    vec4 color;     // rgb: color * intensity, w: range
    vec4 direction; // xyz: direction, w: type (0=dir, 1=point)
};

// ==================== Set 0: 材质数据 ====================
// 布局必须与 Material::UpdateDescriptorSetPBR() 完全一致

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

// ===================== PBR 核心函数 =====================

// Fresnel-Schlick 近似
vec3 FresnelSchlick(vec3 f0, float cosTheta) {
    return f0 + (1.0 - f0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
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

// Smith-Schlick 几何函数
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

// 对单个光源计算 PBR 光照贡献
vec3 CalculatePBRLight(vec3 albedo, float metallic, float roughness,
                       vec3 N, vec3 V, vec3 L, vec3 radiance) {
    vec3 H = normalize(V + L);

    // 漫反射 / 镜面反射能量分配
    vec3 f0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = FresnelSchlick(f0, max(dot(H, V), 0.0));

    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);

    float NdotL = max(dot(N, L), 0.0);

    vec3 diffuse = kD * albedo / PBR_PI;
    vec3 specular = SpecularBRDF(N, V, L, H, roughness, f0);

    return (diffuse + specular) * radiance * NdotL;
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
    vec4 albedoSample = texture(AlbedoMap, v_UV);
    vec3 albedo = material.baseColor.rgb * albedoSample.rgb * v_Color.rgb;
    float alpha = albedoSample.a * material.baseColor.a;

    // Metallic-Roughness (GLTF 规范: B=metallic, G=roughness)
    float metallic = material.metallic;
    float roughness = material.roughness;
    vec4 mrSample = texture(MetallicRoughnessMap, v_UV);
    metallic *= mrSample.b;
    roughness *= mrSample.g;

    // AO (环境光遮蔽)
    float ao = material.ao;
    ao *= texture(AOMap, v_UV).r;

    // 自发光
    vec3 emissive = vec3(0.0);
    if (material.emissiveIntensity > PBR_EPSILON) {
        emissive = texture(EmissiveMap, v_UV).rgb * material.emissiveIntensity;
    }

    // ===== 2. 法线计算 =====
    vec3 N = normalize(v_Normal);

    // ===== 3. 直接光照 =====
    vec3 V = normalize(v_ViewDir);
    vec3 directLighting = vec3(0.0);

    // 遍历所有光源 (最大 MAX_LIGHTS)
    for (int i = 0; i < MAX_LIGHTS; i++) {
        PBRLight light = lightBuffer.lights[i];
        vec3 L = GetLightDir(light, v_WorldPos);
        vec3 radiance = GetLightRadiance(light, v_WorldPos);

        // 跳过无效光源 (无辐射度)
        if (length(radiance) < PBR_EPSILON) continue;

        directLighting += CalculatePBRLight(albedo, metallic, roughness, N, V, L, radiance);
    }

    // ===== 4. 环境光照 (简化: 纯环境光兜底, 无 IBL) =====
    vec3 ambientLighting = albedo * 0.03 * ao;

    // ===== 5. 合成最终颜色 =====
    vec3 finalColor = directLighting + ambientLighting + emissive;

    // ===== 6. 后处理 =====
    finalColor = ACESToneMapping(finalColor, 1.0);
    finalColor = GammaCorrection(finalColor, 2.2);

    outColor = vec4(finalColor, alpha);
}
