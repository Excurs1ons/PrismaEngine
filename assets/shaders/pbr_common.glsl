// ============================================================
// pbr_common.glsl - PrismaEngine PBR 公共函数库
//
// 包含：Cook-Torrance BRDF (GGX)、Fresnel-Schlick、IBL、
//       色调映射、法线贴图解码等通用 PBR 函数。
//
// 注意：GLSL 没有原生的 #include 机制，此文件作为参考文档，
//       各着色器文件直接内联所需函数。
// ============================================================

#ifndef PBR_COMMON_GLSL
#define PBR_COMMON_GLSL

// ===================== 常量定义 =====================

const float PBR_PI = 3.14159265359;
const float PBR_EPSILON = 0.0001;

// ===================== 工具函数 =====================

// 线性化深度 (针对透视投影)
float LinearizeDepth(float depth, float near, float far) {
    return (2.0 * near * far) / (far + near - depth * (far - near));
}

// 从 [0,1] 范围解码法线
vec3 DecodeNormal(vec3 encoded) {
    return encoded * 2.0 - 1.0;
}

// ===================== 核心 PBR BRDF =====================

// Fresnel-Schlick 近似
// f0: 表面在法线方向上的反射率 (vec3 以支持金属色)
// cosTheta: dot(N, V) 或 dot(H, V)
// 返回: 菲涅尔反射率
vec3 FresnelSchlick(vec3 f0, float cosTheta) {
    return f0 + (1.0 - f0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

// 带粗糙度参数的 Fresnel-Schlick (用于 IBL)
vec3 FresnelSchlickRoughness(vec3 f0, float cosTheta, float roughness) {
    return f0 + (max(vec3(1.0 - roughness), f0) - f0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

// GGX Normal Distribution Function (Trowbridge-Reitz)
// alpha: roughness^2
// NdotH: dot(N, H)
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float alpha = max(roughness * roughness, PBR_EPSILON);
    float alpha2 = alpha * alpha;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = NdotH2 * (alpha2 - 1.0) + 1.0;
    denom = max(PBR_PI * denom * denom, PBR_EPSILON);

    return alpha2 / denom;
}

// Smith-Schlick Geometry Function
// k: 粗糙度参数 (直接光照使用 (roughness+1)^2/8, IBL 使用 roughness^2/2)
// NdotV: dot(N, V)
float SchlickGGX(float NdotV, float roughness) {
    float r = max(roughness, PBR_EPSILON);
    float k = (r * r) / 2.0; // IBL 模式
    // float k = (r + 1.0) * (r + 1.0) / 8.0; // 直接光照模式
    float denom = NdotV * (1.0 - k) + k;
    return NdotV / max(denom, PBR_EPSILON);
}

// Smith Geometry Function (联合遮挡和阴影)
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = SchlickGGX(NdotV, roughness);
    float ggx2 = SchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

// Cook-Torrance BRDF 镜面反射项
vec3 SpecularBRDF(vec3 N, vec3 V, vec3 L, vec3 H, float roughness, vec3 f0) {
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = FresnelSchlick(f0, max(dot(H, V), 0.0));

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + PBR_EPSILON;
    return numerator / denominator;
}

// ===================== 完整 PBR 直接光照计算 =====================

struct PBRParams {
    vec3 albedo;
    float metallic;
    float roughness;
    float ao;
    vec3 emissive;
};

struct LightSample {
    vec3 L;          // 归一化的光照方向 (指向光源)
    vec3 radiance;   // 光源辐射度
    float distance;  // 距离 (点/聚光灯)
};

// 对单个光源计算 PBR 光照贡献
vec3 CalculatePBRLight(PBRParams pbr, vec3 N, vec3 V, LightSample light) {
    vec3 L = light.L;
    vec3 H = normalize(V + L);

    // 漫反射和镜面反射能量分配
    vec3 f0 = mix(vec3(0.04), pbr.albedo, pbr.metallic);
    vec3 F = FresnelSchlick(f0, max(dot(H, V), 0.0));

    // 漫反射项 (能量守恒)
    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - pbr.metallic);

    float NdotL = max(dot(N, L), 0.0);

    // 漫反射 (Lambertian)
    vec3 diffuse = kD * pbr.albedo / PBR_PI;

    // 镜面反射 (Cook-Torrance)
    vec3 specular = SpecularBRDF(N, V, L, H, pbr.roughness, f0);

    return (diffuse + specular) * light.radiance * NdotL;
}

// ===================== IBL (Image-Based Lighting) =====================

// 漫反射 IBL: 使用辐照度贴图进行 Lambertian 积分
vec3 IBL_Diffuse(vec3 albedo, float metallic, float ao, vec3 irradiance) {
    vec3 kS = mix(vec3(0.04), albedo, metallic);
    vec3 kD = (1.0 - kS) * (1.0 - metallic);
    return kD * albedo / PBR_PI * irradiance * ao;
}

// 镜面反射 IBL: 使用分割求和近似 (Split-Sum)
// prefilteredColor: 预滤波环境贴图采样结果
// brdfLUT: BRDF 积分查找表采样结果 (scale, bias)
vec3 IBL_Specular(vec3 f0, float roughness, float NdotV, vec3 prefilteredColor, vec2 brdfLUT) {
    vec3 F = FresnelSchlickRoughness(f0, NdotV, roughness);
    return prefilteredColor * (F * brdfLUT.x + brdfLUT.y);
}

// ===================== 光照结构 =====================

// 光源结构 (匹配引擎 Light struct in RenderTypes.h)
struct PBRLight {
    vec4 position;  // xyz: position, w: padding
    vec4 color;     // rgb: color * intensity, w: range
    vec4 direction; // xyz: direction, w: type (0=dir, 1=point, 2=spot)
};

// 获取光源辐射度和方向
LightSample GetLightSample(PBRLight light, vec3 fragPos) {
    LightSample sample;
    int type = int(light.direction.w + 0.5);

    if (type == 0) { // Directional
        sample.L = normalize(-light.direction.xyz);
        sample.radiance = light.color.rgb;
        sample.distance = 1e10;
    } else if (type == 1) { // Point
        vec3 toLight = light.position.xyz - fragPos;
        sample.distance = length(toLight);
        sample.L = toLight / max(sample.distance, PBR_EPSILON);
        float range = light.color.w;
        if (range > 0.0) {
            float attenuation = max(1.0 - (sample.distance * sample.distance) / (range * range), 0.0);
            sample.radiance = light.color.rgb * attenuation * attenuation;
        } else {
            sample.radiance = light.color.rgb;
        }
    } else if (type == 2) { // Spot (simplified)
        vec3 toLight = light.position.xyz - fragPos;
        sample.distance = length(toLight);
        sample.L = toLight / max(sample.distance, PBR_EPSILON);
        float range = light.color.w;
        float spotCos = max(dot(sample.L, normalize(-light.direction.xyz)), 0.0);
        float spotCutoff = cos(radians(30.0));
        float spotSmooth = smoothstep(spotCutoff * 0.8, spotCutoff, spotCos);
        if (range > 0.0) {
            float attenuation = max(1.0 - (sample.distance * sample.distance) / (range * range), 0.0);
            sample.radiance = light.color.rgb * attenuation * attenuation * spotSmooth;
        } else {
            sample.radiance = light.color.rgb * spotSmooth;
        }
    } else {
        sample.L = vec3(0.0);
        sample.radiance = vec3(0.0);
        sample.distance = 0.0;
    }

    return sample;
}

// ===================== 色调映射 =====================

// ACES Filmic Tone Mapping (Narkowicz 拟合)
vec3 ACESToneMapping(vec3 color, float exposure) {
    color *= exposure;
    const float A = 2.51;
    const float B = 0.03;
    const float C = 2.43;
    const float D = 0.59;
    const float E = 0.14;
    return (color * (A * color + B)) / (color * (C * color + D) + E);
}

// Reinhard 色调映射
vec3 ReinhardToneMapping(vec3 color, float exposure) {
    color *= exposure;
    return color / (1.0 + color);
}

// Uncharted 2 色调映射
vec3 Uncharted2ToneMapping(vec3 color, float exposure) {
    color *= exposure;
    const float A = 0.15;
    const float B = 0.50;
    const float C = 0.10;
    const float D = 0.20;
    const float E = 0.02;
    const float F = 0.30;
    return ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
}

// Gamma 校正
vec3 GammaCorrection(vec3 color, float gamma) {
    return pow(max(color, vec3(0.0)), vec3(1.0 / gamma));
}

// ===================== 法线贴图解码 =====================

// 从法线贴图采样并变换到世界空间
vec3 SampleNormalMap(sampler2D normalMap, vec2 uv, mat3 TBN, float normalScale) {
    vec3 normal = texture(normalMap, uv).rgb;
    normal = normal * 2.0 - 1.0;           // 从 [0,1] 解码到 [-1,1]
    normal.xy *= normalScale;               // 法线强度缩放
    normal = normalize(normal);
    return normalize(TBN * normal);
}

// 从法线、切线和副切线构造 TBN 矩阵
mat3 ConstructTBN(vec3 N, vec3 T, vec3 B) {
    vec3 n = normalize(N);
    vec3 t = normalize(T - dot(T, n) * n); // Gram-Schmidt 正交化
    vec3 b = cross(n, t);
    return mat3(t, b, n);
}

#endif // PBR_COMMON_GLSL
