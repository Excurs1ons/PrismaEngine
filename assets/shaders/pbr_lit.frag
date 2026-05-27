// ============================================================
// pbr_lit.frag - PrismaEngine PBR 前向渲染片元着色器
//
// 完整 PBR 光照管线:
//   - Metallic-Roughness 工作流
//   - Cook-Torrance BRDF (GGX)
//   - 多光源支持 (方向光/点光/聚光)
//   - IBL (辐照度 + 预滤波环境 + BRDF LUT)
//   - 法线贴图, AO, 自发光
//   - ACES 色调映射 + Gamma 校正
// ============================================================

#version 450

const float PBR_PI = 3.14159265359;
const float PBR_EPSILON = 0.0001;

// ---- 片元输入 ----
layout(location = 0) in vec3 v_WorldPos;
layout(location = 1) in vec3 v_Normal;
layout(location = 2) in vec2 v_UV;
layout(location = 3) in vec3 v_Tangent;
layout(location = 4) in vec3 v_ViewDir;
layout(location = 5) in vec4 v_Color;

layout(location = 0) out vec4 outColor;

// ---- 光源结构 ----
struct PBRLight {
    vec4 position;  // xyz: position, w: padding
    vec4 color;     // rgb: color * intensity, w: range
    vec4 direction; // xyz: direction, w: type (0=dir, 1=point, 2=spot)
};

// ---- Set 0: 材质数据 ----
layout(set = 0, binding = 0, std140) uniform MaterialData {
    vec4 baseColor;    // rgba
    float metallic;
    float roughness;
    float ao;
    float emissiveIntensity;
} material;

layout(set = 0, binding = 1) uniform sampler2D albedoMap;
layout(set = 0, binding = 2) uniform sampler2D normalMap;
layout(set = 0, binding = 3) uniform sampler2D metallicRoughnessMap;
layout(set = 0, binding = 4) uniform sampler2D aoMap;
layout(set = 0, binding = 5) uniform sampler2D emissiveMap;

// ---- Set 1: 场景数据 ----
layout(set = 1, binding = 0) uniform SceneData {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec4 cameraPos;
} scene;

// ---- Set 2: IBL 环境贴图 ----
layout(set = 2, binding = 0) uniform samplerCube irradianceMap;
layout(set = 2, binding = 1) uniform samplerCube prefilterMap;
layout(set = 2, binding = 2) uniform sampler2D brdfLUT;

// ---- Set 3: 光源数据 (SSBO) ----
layout(set = 3, binding = 0, std430) readonly buffer LightBuffer {
    PBRLight lights[];
} lightBuffer;

layout(set = 3, binding = 1) uniform LightCount {
    uint numLights;
} lightCount;

// ===================== PBR 函数 =====================

vec3 FresnelSchlick(vec3 f0, float cosTheta) {
    return f0 + (1.0 - f0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

vec3 FresnelSchlickRoughness(vec3 f0, float cosTheta, float roughness) {
    return f0 + (max(vec3(1.0 - roughness), f0) - f0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float alpha = max(roughness * roughness, PBR_EPSILON);
    float alpha2 = alpha * alpha;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = NdotH2 * (alpha2 - 1.0) + 1.0;
    denom = max(PBR_PI * denom * denom, PBR_EPSILON);
    return alpha2 / denom;
}

float SchlickGGX(float NdotV, float roughness) {
    float r = max(roughness, PBR_EPSILON);
    float k = (r * r) / 2.0;
    float denom = NdotV * (1.0 - k) + k;
    return NdotV / max(denom, PBR_EPSILON);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return SchlickGGX(NdotV, roughness) * SchlickGGX(NdotL, roughness);
}

vec3 SpecularBRDF(vec3 N, vec3 V, vec3 L, vec3 H, float roughness, vec3 f0) {
    float D = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = FresnelSchlick(f0, max(dot(H, V), 0.0));
    vec3 num = D * G * F;
    float den = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + PBR_EPSILON;
    return num / den;
}

vec3 CalculatePBRLight(vec3 albedo, float metallic, float roughness,
                       vec3 N, vec3 V, vec3 L, vec3 radiance) {
    vec3 H = normalize(V + L);
    vec3 f0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = FresnelSchlick(f0, max(dot(H, V), 0.0));
    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);
    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = kD * albedo / PBR_PI;
    vec3 specular = SpecularBRDF(N, V, L, H, roughness, f0);
    return (diffuse + specular) * radiance * NdotL;
}

vec3 GetLightRadiance(PBRLight light, vec3 fragPos) {
    int type = int(light.direction.w + 0.5);
    vec3 radiance = vec3(0.0);

    if (type == 0) { // Directional
        radiance = light.color.rgb;
    } else if (type == 1) { // Point
        vec3 toLight = light.position.xyz - fragPos;
        float dist = length(toLight);
        float range = light.color.w;
        if (range > 0.0 && dist < range) {
            float atten = max(1.0 - (dist * dist) / (range * range), 0.0);
            radiance = light.color.rgb * atten * atten;
        } else {
            radiance = light.color.rgb;
        }
    } else if (type == 2) { // Spot
        vec3 toLight = light.position.xyz - fragPos;
        float dist = length(toLight);
        vec3 L = normalize(toLight);
        float spotCos = max(dot(L, normalize(-light.direction.xyz)), 0.0);
        float spotCutoff = 0.866; // cos(30°)
        float spotSmooth = smoothstep(spotCutoff * 0.8, spotCutoff, spotCos);
        float range = light.color.w;
        if (range > 0.0 && dist < range) {
            float atten = max(1.0 - (dist * dist) / (range * range), 0.0);
            radiance = light.color.rgb * atten * atten * spotSmooth;
        } else {
            radiance = light.color.rgb * spotSmooth;
        }
    }

    return radiance;
}

vec3 GetLightDir(PBRLight light, vec3 fragPos) {
    int type = int(light.direction.w + 0.5);
    if (type == 0) {
        return normalize(-light.direction.xyz);
    } else {
        return normalize(light.position.xyz - fragPos);
    }
}

// ---- 法线贴图采样 ----
vec3 SampleWorldNormal() {
    vec3 N = normalize(v_Normal);

    // 如果有切线数据, 使用法线贴图
    if (length(v_Tangent) > 0.001 && textureSize(normalMap, 0).x > 1) {
        // 构造 TBN
        vec3 T = normalize(v_Tangent);
        vec3 B = cross(N, T);
        mat3 TBN = mat3(T, B, N);

        // 采样法线贴图
        vec3 normalTS = texture(normalMap, v_UV).rgb;
        normalTS = normalTS * 2.0 - 1.0;
        normalTS = normalize(TBN * normalTS);
        return normalTS;
    }

    return N;
}

// ---- 色调映射 ----
vec3 ACESToneMapping(vec3 color, float exposure) {
    color *= exposure;
    const float A = 2.51;
    const float B = 0.03;
    const float C = 2.43;
    const float D = 0.59;
    const float E = 0.14;
    return (color * (A * color + B)) / (color * (C * color + D) + E);
}

void main() {
    // ===== 1. 采样材质贴图 =====
    vec4 baseColorSample = texture(albedoMap, v_UV);
    vec3 albedo = material.baseColor.rgb * baseColorSample.rgb * v_Color.rgb;

    // Metallic-Roughness (来自贴图或 uniform)
    float metallic = material.metallic;
    float roughness = material.roughness;
    if (textureSize(metallicRoughnessMap, 0).x > 1) {
        vec4 mrSample = texture(metallicRoughnessMap, v_UV);
        metallic *= mrSample.b;    // GLTF 规格: B 通道 = metallic
        roughness *= mrSample.g;   // GLTF 规格: G 通道 = roughness
    }

    // AO
    float ao = material.ao;
    if (textureSize(aoMap, 0).x > 1) {
        ao *= texture(aoMap, v_UV).r;
    }

    // 自发光
    vec3 emissive = vec3(0.0);
    if (material.emissiveIntensity > 0.001) {
        if (textureSize(emissiveMap, 0).x > 1) {
            emissive = texture(emissiveMap, v_UV).rgb * material.emissiveIntensity;
        } else {
            emissive = albedo * material.emissiveIntensity;
        }
    }

    // ===== 2. 法线计算 =====
    vec3 N = SampleWorldNormal();
    vec3 V = normalize(v_ViewDir);

    // ===== 3. 直接光照 =====
    vec3 directLighting = vec3(0.0);

    for (uint i = 0; i < lightCount.numLights; i++) {
        PBRLight light = lightBuffer.lights[i];
        vec3 L = GetLightDir(light, v_WorldPos);
        vec3 radiance = GetLightRadiance(light, v_WorldPos);

        if (length(radiance) < PBR_EPSILON) continue;

        directLighting += CalculatePBRLight(albedo, metallic, roughness, N, V, L, radiance);
    }

    // ===== 4. IBL 环境光照 =====
    vec3 ambientLighting = vec3(0.0);
    vec3 f0 = mix(vec3(0.04), albedo, metallic);

    // 漫反射 IBL
    if (textureSize(irradianceMap, 0).x > 1) {
        vec3 irradiance = texture(irradianceMap, N).rgb;
        vec3 kD_ibl = (1.0 - FresnelSchlickRoughness(f0, max(dot(N, V), 0.0), roughness)) * (1.0 - metallic);
        ambientLighting += kD_ibl * albedo / PBR_PI * irradiance * ao;
    }

    // 镜面反射 IBL
    if (textureSize(prefilterMap, 0).x > 1 && textureSize(brdfLUT, 0).x > 1) {
        vec3 R = reflect(-V, N);
        float lod = roughness * 4.0; // 假设预滤波 mip 级别为 0-4
        vec3 prefilteredColor = textureLod(prefilterMap, R, lod).rgb;

        vec2 envBRDF = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
        vec3 fresnelIBL = FresnelSchlickRoughness(f0, max(dot(N, V), 0.0), roughness);
        ambientLighting += prefilteredColor * (fresnelIBL * envBRDF.x + envBRDF.y);
    }

    // 如果没有 IBL, 使用纯环境光兜底
    if (ambientLighting.r < 0.001 && ambientLighting.g < 0.001 && ambientLighting.b < 0.001) {
        ambientLighting = albedo * 0.03 * ao;
    }

    // ===== 5. 合成最终颜色 =====
    vec3 finalColor = directLighting + ambientLighting + emissive;

    // ===== 6. 后处理 =====
    finalColor = ACESToneMapping(finalColor, 1.0);
    finalColor = pow(max(finalColor, vec3(0.0)), vec3(1.0 / 2.2));

    outColor = vec4(finalColor, baseColorSample.a * material.baseColor.a);
}
