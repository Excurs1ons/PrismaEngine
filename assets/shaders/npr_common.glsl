// ============================================================
// npr_common.glsl - PrismaEngine NPR 公共函数库
//
// 包含：Toon/Cel 漫反射渐变、软菲涅尔边缘光、风格化高光、
//       暖色调环境光、NPR 色调映射、暗角辅助。
//
// 美学参考：Sky: Children of the Light (暖色调 + 柔光风格)
//
// 注意：GLSL 没有原生的 #include 机制，此文件作为参考文档，
//       各着色器文件直接内联所需函数。
// ============================================================

#ifndef NPR_COMMON_GLSL
#define NPR_COMMON_GLSL

// ===================== 常量定义 =====================

const float NPR_PI = 3.14159265359;
const float NPR_EPSILON = 0.0001;

// 阴影色暖色调 (Sky 风格: 温暖棕色阴影)
const vec3 NPR_SHADOW_TINT = vec3(0.3, 0.2, 0.15);

// ===================== Toon 漫反射渐变 =====================

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

// ===================== 菲涅尔边缘光 =====================

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

// ===================== 风格化高光 =====================

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

// ===================== 暖色调环境光 =====================

// 三色混合环境光 (ground/sky/horizon)
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

// ===================== NPR 色调映射 =====================

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

// ===================== 暗角辅助 =====================

// 基于屏幕 UV 的暗角效果
// uv:        屏幕 UV 坐标 [0,1]
// strength:  暗角强度
// 返回: [0,1], 1.0 = 无暗角
float NPR_Vignette(vec2 uv, float strength) {
    vec2 center = uv - 0.5;
    float vignette = dot(center, center);
    return 1.0 - vignette * strength;
}

#endif // NPR_COMMON_GLSL
