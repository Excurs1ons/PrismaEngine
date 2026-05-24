// ============================================================
// pbr_deferred_lighting.frag - PrismaEngine PBR 延迟光照着色器
//
// 从 G-Buffer 读取数据进行完整 PBR 光照计算:
//   - Cook-Torrance BRDF (GGX)
//   - 多光源支持 (方向光/点光/聚光)
//   - IBL (辐照度 + 预滤波环境 + BRDF LUT)
//   - ACES 色调映射 + Gamma 校正
// ============================================================

#version 450

const float PBR_PI = 3.14159265359;
const float PBR_EPSILON = 0.0001;

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

// ---- G-Buffer 贴图 ----
layout(binding = 0) uniform sampler2D gbPosition;    // RGB: 世界位置, A: 粗糙度
layout(binding = 1) uniform sampler2D gbNormal;      // RGB: 编码法线, A: 金属度
layout(binding = 2) uniform sampler2D gbAlbedo;      // RGB: 反照率, A: AO
layout(binding = 3) uniform sampler2D gbEmissive;    // RGB: 自发光, A: 材质标志

// ---- IBL 环境贴图 ----
layout(binding = 4) uniform samplerCube irradianceMap;
layout(binding = 5) uniform samplerCube prefilterMap;
layout(binding = 6) uniform sampler2D brdfLUT;

// ---- 深度贴图 (用于世界空间重建) ----
layout(binding = 7) uniform sampler2D gbDepth;

// ---- Push Constants: 光照参数 ----
layout(push_constant) uniform PushConstants {
    vec4 cameraPos;    // xyz: 相机位置, w: padding
} pc;

// ---- 光源结构 (与引擎 Light 结构匹配) ----
struct PBRLight {
    vec4 position;  // xyz: position, w: padding
    vec4 color;     // rgb: color * intensity, w: range
    vec4 direction; // xyz: direction, w: type (0=dir, 1=point, 2=spot)
};

// ---- 光源 SSBO (Set 0, 固定绑定 0) ----
// 注意: Set 自动递增, 取决于 G-Buffer 的 binding 数量
layout(set = 1, binding = 0, std430) readonly buffer LightBuffer {
    PBRLight lights[];
} lightBuffer;

layout(set = 1, binding = 1) uniform LightCount {
    uint numLights;
    float exposure;
    float gamma;
    float envLightIntensity;
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

    if (type == 0) { // Directional
        return light.color.rgb;
    } else if (type == 1) { // Point
        vec3 toLight = light.position.xyz - fragPos;
        float dist = length(toLight);
        float range = light.color.w;
        if (range > 0.0 && dist < range) {
            float atten = max(1.0 - (dist * dist) / (range * range), 0.0);
            return light.color.rgb * atten * atten;
        }
        return light.color.rgb;
    } else if (type == 2) { // Spot
        vec3 toLight = light.position.xyz - fragPos;
        float dist = length(toLight);
        vec3 L = normalize(toLight);
        float spotCos = max(dot(L, normalize(-light.direction.xyz)), 0.0);
        float spotCutoff = 0.866;
        float spotSmooth = smoothstep(spotCutoff * 0.8, spotCutoff, spotCos);
        float range = light.color.w;
        if (range > 0.0 && dist < range) {
            float atten = max(1.0 - (dist * dist) / (range * range), 0.0);
            return light.color.rgb * atten * atten * spotSmooth;
        }
        return light.color.rgb * spotSmooth;
    }

    return vec3(0.0);
}

vec3 GetLightDir(PBRLight light, vec3 fragPos) {
    int type = int(light.direction.w + 0.5);
    if (type == 0) {
        return normalize(-light.direction.xyz);
    } else {
        return normalize(light.position.xyz - fragPos);
    }
}

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
    // ===== 1. 从 G-Buffer 采样 =====
    vec4 posData = texture(gbPosition, inUV);
    vec4 normalData = texture(gbNormal, inUV);
    vec4 albedoData = texture(gbAlbedo, inUV);
    vec4 emissiveData = texture(gbEmissive, inUV);

    // 跳过空白像素 (天空盒/背景)
    if (length(posData.xyz) < 0.001) {
        discard;
    }

    vec3 worldPos = posData.xyz;
    float roughness = posData.w;
    vec3 N = normalize(normalData.xyz * 2.0 - 1.0); // 解码法线
    float metallic = normalData.w;
    vec3 albedo = albedoData.rgb;
    float ao = albedoData.w;
    vec3 emissive = emissiveData.rgb;

    vec3 V = normalize(pc.cameraPos.xyz - worldPos);

    // ===== 2. 直接光照 =====
    vec3 directLighting = vec3(0.0);

    for (uint i = 0; i < lightCount.numLights; i++) {
        PBRLight light = lightBuffer.lights[i];
        vec3 L = GetLightDir(light, worldPos);
        vec3 radiance = GetLightRadiance(light, worldPos);

        if (length(radiance) < PBR_EPSILON) continue;

        directLighting += CalculatePBRLight(albedo, metallic, roughness, N, V, L, radiance);
    }

    // ===== 3. IBL 环境光照 =====
    vec3 ambientLighting = vec3(0.0);
    vec3 f0 = mix(vec3(0.04), albedo, metallic);
    float envIntensity = lightCount.envLightIntensity;

    // 漫反射 IBL
    if (textureSize(irradianceMap, 0).x > 1) {
        vec3 irradiance = texture(irradianceMap, N).rgb * envIntensity;
        float NdotV_ibl = max(dot(N, V), 0.0);
        vec3 kD_ibl = (1.0 - FresnelSchlickRoughness(f0, NdotV_ibl, roughness)) * (1.0 - metallic);
        ambientLighting += kD_ibl * albedo / PBR_PI * irradiance * ao;
    }

    // 镜面反射 IBL
    if (textureSize(prefilterMap, 0).x > 1 && textureSize(brdfLUT, 0).x > 1) {
        vec3 R = reflect(-V, N);
        float lod = roughness * 4.0;
        vec3 prefilteredColor = textureLod(prefilterMap, R, lod).rgb * envIntensity;
        vec2 envBRDF = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
        vec3 fresnelIBL = FresnelSchlickRoughness(f0, max(dot(N, V), 0.0), roughness);
        ambientLighting += prefilteredColor * (fresnelIBL * envBRDF.x + envBRDF.y);
    }

    // 如果没有 IBL, 使用环境光兜底
    if (ambientLighting.r < 0.001 && ambientLighting.g < 0.001 && ambientLighting.b < 0.001) {
        ambientLighting = albedo * 0.03 * ao;
    }

    // ===== 4. 合成最终颜色 =====
    vec3 finalColor = directLighting + ambientLighting + emissive;

    // ===== 5. 后处理 =====
    finalColor = ACESToneMapping(finalColor, lightCount.exposure);
    finalColor = pow(max(finalColor, vec3(0.0)), vec3(1.0 / lightCount.gamma));

    outColor = vec4(finalColor, 1.0);
}
