// ============================================================
// npr_deferred_lighting.frag - PrismaEngine NPR 延迟光照着色器
//
// 从 G-Buffer 读取数据进行 NPR/Toon 风格光照计算:
//   - Toon 漫反射渐变 (4 级色带, 暖色阴影)
//   - Blinn-Phong 风格化高光 (柔化边缘)
//   - 软菲涅尔边缘光 (Sky: Children of the Light 风格)
//   - 三色混合暖色调环境光 (ground/sky/horizon)
//   - Reinhard 色调映射 + 暖色偏移
// ============================================================

#version 450

const float NPR_PI = 3.14159265359;
const float NPR_EPSILON = 0.0001;

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

// ---- G-Buffer 贴图 ----
layout(binding = 0) uniform sampler2D gbAlbedo;      // rgb: 反照率, a: NdotV (GBuffer 预计算)
layout(binding = 1) uniform sampler2D gbNormal;       // rgb: 编码法线, a: 金属度标记
layout(binding = 2) uniform sampler2D gbNPRParams;    // r: rimPower, g: wrapAmount, b: shadowIntensity
layout(binding = 3) uniform sampler2D gbDepth;        // 深度 (用于世界空间重建)

// ---- 相机逆 VP 矩阵 (用于深度重建世界坐标) ----
layout(binding = 4) uniform CameraData {
    mat4 invViewProj;
} camera;

// ---- Push Constants: 相机位置 ----
layout(push_constant) uniform PushConstants {
    vec4 cameraPos;    // xyz: 相机位置, w: padding
} pc;

// ---- 光源结构 (与引擎 Light 结构匹配) ----
struct PBRLight {
    vec4 position;  // xyz: position, w: padding
    vec4 color;     // rgb: color * intensity, w: range
    vec4 direction; // xyz: direction, w: type (0=dir, 1=point, 2=spot)
};

// ---- 光源 SSBO (Set 1, 固定绑定 0-1) ----
layout(set = 1, binding = 0, std430) readonly buffer LightBuffer {
    PBRLight lights[];
} lightBuffer;

layout(set = 1, binding = 1) uniform LightCount {
    uint numLights;
    float exposure;
    float gamma;
    float envLightIntensity;
} lightCount;

// ===================== NPR 常量 =====================

// 阴影色暖色调 (Sky 风格: 温暖棕色阴影)
const vec3 NPR_SHADOW_TINT = vec3(0.3, 0.2, 0.15);

// 环境光颜色 (Sky 风格)
const vec3 NPR_AMBIENT_GROUND  = vec3(0.08, 0.06, 0.10);
const vec3 NPR_AMBIENT_SKY     = vec3(0.35, 0.30, 0.40);
const vec3 NPR_AMBIENT_HORIZON = vec3(0.55, 0.40, 0.25);

// 默认 NPR 材质参数 (不在 G-Buffer 中时使用 fallback)
const float NPR_DEFAULT_SHININESS   = 32.0;
const float NPR_DEFAULT_SMOOTHNESS  = 0.3;
const float NPR_DEFAULT_RIM_STRENGTH = 0.6;
const float NPR_DEFAULT_HORIZON_HEIGHT = 0.25;

// ===================== Toon 漫反射渐变 =====================

vec3 NPR_DiffuseRamp(vec3 albedo, vec3 lightColor, float NdotL, float wrap, float shadowIntensity) {
    // wrap 光照: 模拟次表面散射
    float wrappedNdotL = max((NdotL + wrap) / (1.0 + wrap), 0.0);

    // 定义 4 级渐变阈值
    const float RAMP_SHADOW     = 0.1;
    const float RAMP_HALF       = 0.35;
    const float RAMP_LIGHT      = 0.6;

    // 计算渐变值
    float ramp;
    if (wrappedNdotL < RAMP_SHADOW) {
        ramp = 0.0;
    } else if (wrappedNdotL < RAMP_HALF) {
        ramp = smoothstep(RAMP_SHADOW, RAMP_HALF, wrappedNdotL) * 0.5;
    } else if (wrappedNdotL < RAMP_LIGHT) {
        ramp = 0.5 + smoothstep(RAMP_HALF, RAMP_LIGHT, wrappedNdotL) * 0.35;
    } else {
        ramp = 0.85 + smoothstep(RAMP_LIGHT, 1.0, wrappedNdotL) * 0.15;
    }

    // 暖色阴影 tint + 漫反射颜色
    vec3 shadedColor = mix(NPR_SHADOW_TINT * albedo, vec3(0.0), 1.0 - shadowIntensity);
    vec3 litColor = albedo * lightColor;
    vec3 finalColor = mix(shadedColor, litColor, ramp);

    return finalColor;
}

// ===================== 菲涅尔边缘光 =====================

vec3 NPR_RimLight(vec3 rimColor, float NdotV, float rimPower, float rimStrength) {
    float rim = pow(max(1.0 - NdotV, 0.0), rimPower);
    return rimColor * rim * rimStrength;
}

// ===================== 风格化高光 =====================

float NPR_Specular(vec3 N, vec3 H, float shininess, float smoothness) {
    float NdotH = max(dot(N, H), 0.0);
    float spec = pow(NdotH, shininess);

    // 使用 smoothstep 柔化高光边缘
    float threshold = 0.5 - smoothness * 0.4;
    spec = smoothstep(threshold, threshold + smoothness, spec);

    return spec;
}

// ===================== 暖色调环境光 =====================

vec3 NPR_HorizonAmbient(vec3 N, vec3 V, vec3 groundColor, vec3 skyColor,
                        vec3 horizonColor, float horizonHeight) {
    float heightFactor = dot(N, V) * 0.5 + 0.5;

    float horizonWeight = exp(-pow((heightFactor - 0.5) / horizonHeight, 2.0));

    vec3 ambient = mix(groundColor, skyColor, heightFactor);
    ambient = mix(ambient, horizonColor, horizonWeight);

    return ambient;
}

// ===================== NPR 色调映射 =====================

vec3 NPR_ToneMap(vec3 color, float exposure) {
    color = color * exposure;
    color = color / (color + vec3(1.0));
    color = mix(color, color * vec3(1.0, 0.95, 0.85), 0.3);
    return color;
}

// ===================== 光源函数 (从 PBR 复制) =====================

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

// ===================== 世界坐标重建 =====================

vec3 ReconstructWorldPos(vec2 uv, float depth, mat4 invViewProj) {
    vec4 ndc = vec4(uv * 2.0 - 1.0, depth, 1.0);
    vec4 world = invViewProj * ndc;
    return world.xyz / world.w;
}

// ===================== 主函数 =====================

void main() {
    // ===== 1. 从 G-Buffer 采样 =====
    vec4 albedoData = texture(gbAlbedo, inUV);
    vec4 normalData = texture(gbNormal, inUV);
    vec4 nprParams  = texture(gbNPRParams, inUV);
    float depth     = texture(gbDepth, inUV).r;

    // 跳过空白像素 (天空盒/背景)
    if (depth > 0.999 || length(albedoData.rgb) < 0.001) {
        discard;
    }

    vec3 albedo   = albedoData.rgb;
    float gbNdotV = albedoData.a;

    // 解码法线
    vec3 N = normalize(normalData.xyz * 2.0 - 1.0);

    // NPR 材质参数
    float rimPower       = nprParams.r;
    float wrapAmount     = nprParams.g;
    float shadowIntensity = nprParams.b;

    // ===== 2. 重建世界坐标和视线方向 =====
    vec3 worldPos = ReconstructWorldPos(inUV, depth, camera.invViewProj);
    vec3 V = normalize(pc.cameraPos.xyz - worldPos);

    // NdotV: 优先使用 GBuffer 预计算值
    float NdotV = max(dot(N, V), 0.0);
    if (gbNdotV > 0.0) {
        NdotV = gbNdotV;
    }

    // ===== 3. 直接光照 (逐光源) =====
    vec3 directLighting = vec3(0.0);

    for (uint i = 0; i < lightCount.numLights; i++) {
        PBRLight light = lightBuffer.lights[i];
        vec3 L = GetLightDir(light, worldPos);
        vec3 radiance = GetLightRadiance(light, worldPos);

        if (length(radiance) < NPR_EPSILON) continue;

        // ---- 漫反射: Toon 渐变 ----
        float NdotL = max(dot(N, L), 0.0);
        vec3 diffuse = NPR_DiffuseRamp(albedo, radiance, NdotL, wrapAmount, shadowIntensity);

        // ---- 高光: Blinn-Phong 风格化 ----
        vec3 H = normalize(V + L);
        float spec = NPR_Specular(N, H, NPR_DEFAULT_SHININESS, NPR_DEFAULT_SMOOTHNESS);
        vec3 specular = albedo * radiance * spec * 0.5;

        directLighting += diffuse + specular;
    }

    // ===== 4. 边缘光 =====
    vec3 rimColor = vec3(1.0, 0.7, 0.4); // 暖色边缘光
    vec3 rimLight = NPR_RimLight(rimColor, NdotV, rimPower, NPR_DEFAULT_RIM_STRENGTH);

    // ===== 5. 环境光 =====
    vec3 ambient = NPR_HorizonAmbient(N, V,
        NPR_AMBIENT_GROUND,
        NPR_AMBIENT_SKY,
        NPR_AMBIENT_HORIZON,
        NPR_DEFAULT_HORIZON_HEIGHT);
    vec3 ambientLighting = ambient * albedo * lightCount.envLightIntensity;

    // ===== 6. 合成 =====
    vec3 finalColor = directLighting + rimLight + ambientLighting;

    // ===== 7. 后处理: NPR 色调映射 + Gamma =====
    finalColor = NPR_ToneMap(finalColor, lightCount.exposure);
    finalColor = pow(max(finalColor, vec3(0.0)), vec3(1.0 / lightCount.gamma));

    outColor = vec4(finalColor, 1.0);
}
