// ============================================================
// npr_lit.frag - PrismaEngine NPR 前向渲染片元着色器
//
// NPR 光照管线:
//   - Toon/Cel 漫反射渐变 (wrap 光照模拟次表面散射)
//   - Blinn-Phong 风格化高光
//   - 软菲涅尔边缘光
//   - 三色暖色调环境光 (ground/sky/horizon)
//   - 多光源支持 (方向光/点光/聚光)
//   - Gran Turismo 风格色调映射 + Gamma 校正
//
// 美学参考: Sky: Children of the Light
// ============================================================

#version 450

const float NPR_PI = 3.14159265359;
const float NPR_EPSILON = 0.0001;

// 阴影色暖色调 (Sky 风格: 温暖棕色阴影)
const vec3 NPR_SHADOW_TINT = vec3(0.3, 0.2, 0.15);

// ---- 片元输入 ----
layout(location = 0) in vec3 v_WorldPos;
layout(location = 1) in vec3 v_Normal;
layout(location = 2) in vec2 v_UV;
layout(location = 3) in vec3 v_Tangent;
layout(location = 4) in vec3 v_ViewDir;
layout(location = 5) in vec4 v_Color;

layout(location = 0) out vec4 outColor;

// ---- 光源结构 (与 PBR 共用) ----
struct PBRLight {
    vec4 position;  // xyz: position, w: padding
    vec4 color;     // rgb: color * intensity, w: range
    vec4 direction; // xyz: direction, w: type (0=dir, 1=point, 2=spot)
};

// ---- Set 0: NPR 材质数据 ----
layout(set = 0, binding = 0, std140) uniform NPRMaterialData {
    vec4 baseColor;      // rgba
    vec4 rimColor;       // rgb + intensity in w
    vec4 shadowColor;    // shadow tint (rgb, w=strength)
    vec4 horizonColor;   // warm horizon (rgb, w=strength)
    float roughness;     // remapped to toon spec size
    float rimPower;      // rim light width
    float wrapAmount;    // diffuse wrap for SSS
    float emissiveIntensity;
} material;

layout(set = 0, binding = 1) uniform sampler2D albedoMap;
layout(set = 0, binding = 2) uniform sampler2D normalMap;
layout(set = 0, binding = 3) uniform sampler2D metallicRoughnessMap;

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

// ===================== 光源工具函数 (来自 PBR) =====================

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

// ===================== 法线贴图采样 (来自 PBR) =====================

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

// ===================== NPR 函数 =====================

// ---- Toon 漫反射渐变 ----
// 3-4 级明暗色带的 Toon 漫反射
// albedo:      基础颜色
// lightColor:  光源颜色
// NdotL:       dot(N, L)
// wrap:        次表面散射模拟参数 [0,1]
// 返回: 漫反射光照颜色 (含渐变过渡)
vec3 NPR_DiffuseRamp(vec3 albedo, vec3 lightColor, float NdotL, float wrap) {
    // wrap 光照: 模拟次表面散射
    NdotL = max((NdotL + wrap) / (1.0 + wrap), 0.0);

    // 定义 4 级渐变阈值
    const float RAMP_SHADOW     = 0.1;   // 阴影区
    const float RAMP_HALF       = 0.35;  // 半影区
    const float RAMP_LIGHT      = 0.6;   // 亮部区

    // 计算渐变值
    float ramp;
    if (NdotL < RAMP_SHADOW) {
        // 全阴影: 使用暖色阴影 tint
        ramp = 0.0;
    } else if (NdotL < RAMP_HALF) {
        // 阴影 -> 半影 (平滑过渡)
        ramp = smoothstep(RAMP_SHADOW, RAMP_HALF, NdotL) * 0.5;
    } else if (NdotL < RAMP_LIGHT) {
        // 半影 -> 亮部 (平滑过渡)
        ramp = 0.5 + smoothstep(RAMP_HALF, RAMP_LIGHT, NdotL) * 0.35;
    } else {
        // 亮部 -> 高亮 (平滑过渡)
        ramp = 0.85 + smoothstep(RAMP_LIGHT, 1.0, NdotL) * 0.15;
    }

    // 暖色阴影 tint + 漫反射颜色
    vec3 shadedColor = NPR_SHADOW_TINT * albedo;
    vec3 litColor = albedo * lightColor;
    vec3 finalColor = mix(shadedColor, litColor, ramp);

    return finalColor;
}

// ---- 菲涅尔边缘光 ----
// 软菲涅尔边缘光 (Sky 风格)
// rimColor:    边缘光颜色 (通常为暖色)
// NdotV:       dot(N, V)
// rimPower:    边缘光宽度 (值越小越宽)
// rimStrength: 边缘光强度
// 返回: 边缘光颜色
vec3 NPR_RimLight(vec3 rimColor, float NdotV, float rimPower, float rimStrength) {
    float rim = pow(max(1.0 - NdotV, 0.0), rimPower);
    return rimColor * rim * rimStrength;
}

// ---- 风格化高光 ----
// Blinn-Phong 风格化高光 (非 GGX)
// N:          法线
// H:          半向量 (normalize(V + L))
// shininess:  光泽度 (值越高高光越集中)
// smoothness: 高光边缘平滑度
// 返回: [0,1] 高光强度
float NPR_Specular(vec3 N, vec3 H, float shininess, float smoothness) {
    float NdotH = max(dot(N, H), 0.0);
    float spec = pow(NdotH, shininess);

    // 使用 smoothstep 柔化高光边缘
    float threshold = 0.5 - smoothness * 0.4;
    spec = smoothstep(threshold, threshold + smoothness, spec);

    return spec;
}

// ---- 三色混合环境光 (ground/sky/horizon) ----
// 参考 Sky 标志性暖色调水平线
// N:            法线
// V:            视线方向
// groundColor:  地面环境色
// skyColor:     天空环境色
// horizonColor: 水平线环境色 (暖色调)
// horizonHeight:水平线影响范围
// 返回: 环境光照颜色
vec3 NPR_HorizonAmbient(vec3 N, vec3 V, vec3 groundColor, vec3 skyColor, vec3 horizonColor, float horizonHeight) {
    // 将 NdotV 映射到 [0,1]
    float heightFactor = dot(N, V) * 0.5 + 0.5;

    // 计算水平线权重: 水平线附近最强
    float horizonWeight = exp(-pow((heightFactor - 0.5) / horizonHeight, 2.0));

    // 混合 ground/sky
    vec3 ambient = mix(groundColor, skyColor, heightFactor);

    // 插入水平线颜色
    ambient = mix(ambient, horizonColor, horizonWeight);

    return ambient;
}

// ---- NPR 色调映射 ----
// Gran Turismo 风格色调映射 (保留饱和度, 柔和 clip 高光)
// color:     HDR 颜色
// exposure:  曝光值
// 返回: LDR 颜色 (带暖色偏移)
vec3 NPR_ToneMap(vec3 color, float exposure) {
    // 曝光
    color = color * exposure;

    // Reinhard 风格映射 (保留饱和度)
    color = color / (color + vec3(1.0));

    // 暖色偏移: 在暗部和中间调增加暖色
    color = mix(color, color * vec3(1.0, 0.95, 0.85), 0.3);

    return color;
}

// ===================== 主函数 =====================

void main() {
    // ===== 1. 采样材质贴图 =====
    vec4 albedoSample = texture(albedoMap, v_UV);
    vec3 baseColor = material.baseColor.rgb * albedoSample.rgb * v_Color.rgb;
    float alpha = material.baseColor.a * albedoSample.a;

    // Roughness (重映射为 Toon 高光尺寸)
    float roughness = material.roughness;
    if (textureSize(metallicRoughnessMap, 0).x > 1) {
        vec4 mrSample = texture(metallicRoughnessMap, v_UV);
        roughness *= mrSample.g; // GLTF: G 通道 = roughness
    }

    // ===== 2. 法线计算 =====
    vec3 N = SampleWorldNormal();
    vec3 V = normalize(v_ViewDir);

    // ===== 3. 直接光照 (Toon) =====
    vec3 directLighting = vec3(0.0);

    for (uint i = 0; i < lightCount.numLights; i++) {
        PBRLight light = lightBuffer.lights[i];
        vec3 L = GetLightDir(light, v_WorldPos);
        vec3 radiance = GetLightRadiance(light, v_WorldPos);

        if (length(radiance) < NPR_EPSILON) continue;

        float NdotL = dot(N, L);

        // Toon 漫反射渐变
        vec3 diffuse = NPR_DiffuseRamp(baseColor, radiance, NdotL, material.wrapAmount);

        // 风格化高光 (Blinn-Phong)
        vec3 H = normalize(V + L);
        float shininess = mix(128.0, 2.0, roughness);
        float spec = NPR_Specular(N, H, shininess, 0.3);
        vec3 specular = spec * radiance * 0.5;

        directLighting += diffuse + specular;
    }

    // ===== 4. 菲涅尔边缘光 =====
    float NdotV = max(dot(N, V), 0.0);
    vec3 rim = NPR_RimLight(material.rimColor.rgb, NdotV, material.rimPower, material.rimColor.w);

    // ===== 5. 暖色调环境光 =====
    // groundColor = shadowColor tint, skyColor = albedo, horizonColor = warm horizon
    vec3 ambient = NPR_HorizonAmbient(N, V,
        material.shadowColor.rgb,
        baseColor,
        material.horizonColor.rgb,
        0.3);

    // ===== 6. 自发光 =====
    vec3 emissive = vec3(0.0);
    if (material.emissiveIntensity > 0.001) {
        emissive = baseColor * material.emissiveIntensity;
    }

    // ===== 7. 合成最终颜色 =====
    vec3 finalColor = directLighting + rim + ambient + emissive;

    // ===== 8. 色调映射 + Gamma 校正 =====
    finalColor = NPR_ToneMap(finalColor, 1.0);
    finalColor = pow(max(finalColor, vec3(0.0)), vec3(1.0 / 2.2));

    outColor = vec4(finalColor, alpha);
}
