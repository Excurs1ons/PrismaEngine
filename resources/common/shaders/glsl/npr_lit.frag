#version 460 core
// ============================================================
// npr_lit.frag — PrismaEngine NPR 前向渲染片元着色器
//
// NPR (Non-Photorealistic Rendering) 风格化着色:
//   - 三色调渐变漫反射 (亮/中/暗)
//   - 平滑高光 (Phong)
//   - 边缘光 (Rim Light)
//   - 赛璐珞风格阴影
//   - 基于 wrap lighting 的半阴影过渡
//   - 颜色量化 (减少色带)
// ============================================================

#define NPR_PI 3.14159265359
#define NPR_EPSILON 0.0001

// ---- 片元输入 (来自 npr_lit.vert) ----
layout(location = 0) in vec3 v_WorldPos;
layout(location = 1) in vec3 v_Normal;
layout(location = 2) in vec2 v_TexCoord;
layout(location = 3) in vec3 v_ViewDir;
layout(location = 4) in vec4 v_Color;

layout(location = 0) out vec4 outColor;

// ---- 光源结构 ----
struct NPRLight {
    vec4 position;  // xyz: position, w: type (0=dir, 1=point)
    vec4 color;     // rgb: color * intensity, w: range
    vec4 direction; // xyz: direction, w: padding
};

// ==================== Set 0: NPR 材质数据 ====================
// 布局必须与 Material::NPRMaterialData 一致

layout(set = 0, binding = 0, std140) uniform NPRMaterialData {
    vec4 baseColor;
    vec4 rimColor;
    vec4 shadowColor;
    vec4 horizonColor;
    float roughness;
    float rimPower;
    float wrapAmount;
    float emissiveIntensity;
    float bloomThreshold;
    float padding[3];
} material;

layout(set = 0, binding = 1) uniform sampler2D AlbedoMap;
layout(set = 0, binding = 2) uniform sampler2D NormalMap;
layout(set = 0, binding = 3) uniform sampler2D MetallicRoughnessMap;

// ==================== Set 1: 场景数据 ====================

layout(set = 1, binding = 0, std140) uniform SceneData {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec4 cameraPos;
} scene;

// ==================== Set 3: 光源数据 ====================

layout(set = 3, binding = 0, std430) readonly buffer NPRLightBuffer {
    NPRLight lights[];
} lightBuffer;

layout(set = 3, binding = 1, std140) uniform NPRLightCount {
    uint numLights;
} lightCount;

// ===================== NPR 核心函数 =====================

// Wrap Lighting: 扩展光照范围, 产生柔和过渡
float WrapDot(vec3 N, vec3 L, float wrap) {
    float dotNL = dot(N, L);
    return max((dotNL + wrap) / (1.0 + wrap), 0.0);
}

// 三色调步函数: 将连续光照值映射到 3 个离散色阶
vec3 ToneStep(float lightAmount, vec3 lightColor, vec3 midColor, vec3 shadowColor) {
    // 亮部: > 0.7
    if (lightAmount > 0.7) {
        return lightColor;
    }
    // 中间调: > 0.3
    if (lightAmount > 0.3) {
        return mix(shadowColor, lightColor, (lightAmount - 0.3) / 0.4);
    }
    // 暗部: < 0.3
    return shadowColor;
}

// 平滑高光 (Phong)
float SmoothSpecular(vec3 N, vec3 H, float roughness) {
    float NdotH = max(dot(N, H), 0.0);
    float shininess = mix(1.0, 256.0, 1.0 - roughness);
    return pow(NdotH, shininess);
}

// 边缘光
float CalcRimLight(vec3 N, vec3 V, float rimPower) {
    float NdotV = max(1.0 - dot(N, V), 0.0);
    return pow(NdotV, rimPower);
}

// 赛璐珞阴影: 使用硬步进
float CelStep(float value, float threshold) {
    return step(threshold, value);
}

// ===================== 色调映射 (柔和版) =====================

vec3 SimpleToneMap(vec3 color) {
    return color / (color + vec3(1.0));
}

vec3 GammaCorrection(vec3 color, float gamma) {
    return pow(max(color, vec3(0.0)), vec3(1.0 / gamma));
}

// ===================== 主函数 =====================

void main() {
    // ===== 1. 采样材质贴图 =====
    vec4 albedoSample = texture(AlbedoMap, v_TexCoord);
    vec3 baseColor = material.baseColor.rgb * albedoSample.rgb * v_Color.rgb;
    float alpha = albedoSample.a * material.baseColor.a;

    // ===== 2. 法线 =====
    vec3 N = normalize(v_Normal);

    // ===== 3. 视角方向 =====
    vec3 V = normalize(v_ViewDir);

    // ===== 4. 主光源方向 (从第一个光源获取) =====
    vec3 mainLightDir = vec3(0.0, -1.0, 0.0);
    vec3 mainLightColor = vec3(1.0);
    if (lightCount.numLights > 0) {
        NPRLight mainLight = lightBuffer.lights[0];
        int type = int(mainLight.position.w + 0.5);
        if (type == 0) {
            mainLightDir = normalize(-mainLight.direction.xyz);
        } else {
            mainLightDir = normalize(mainLight.position.xyz - v_WorldPos);
        }
        mainLightColor = mainLight.color.rgb;
    }

    // ===== 5. Wrap Lighting =====
    float wrapDot = WrapDot(N, mainLightDir, material.wrapAmount);

    // ===== 6. 三色调漫反射 =====
    vec3 diffuseLight = ToneStep(wrapDot, baseColor, material.horizonColor.rgb, material.shadowColor.rgb);

    // ===== 7. 高光 =====
    vec3 H = normalize(mainLightDir + V);
    float spec = SmoothSpecular(N, H, material.roughness);
    // 高光阈值 (赛璐珞风格)
    float celSpec = CelStep(spec, 0.3);
    vec3 specularLight = vec3(celSpec) * mainLightColor * 0.5;

    // ===== 8. 边缘光 =====
    float rim = CalcRimLight(N, V, material.rimPower);
    vec3 rimLight = material.rimColor.rgb * rim * mainLightColor;

    // ===== 9. 自发光 =====
    vec3 emissive = vec3(0.0);
    if (material.emissiveIntensity > NPR_EPSILON) {
        emissive = baseColor * material.emissiveIntensity;
    }

    // ===== 10. 合成 =====
    vec3 finalColor = diffuseLight * mainLightColor + specularLight + rimLight + emissive;

    // ===== 11. 颜色量化 (减少色带) =====
    // 将颜色量化为每个通道 8 级
    const float quantizeLevels = 8.0;
    finalColor = floor(finalColor * quantizeLevels) / quantizeLevels;

    // ===== 12. 后处理 =====
    finalColor = SimpleToneMap(finalColor);
    finalColor = GammaCorrection(finalColor, 2.2);

    outColor = vec4(finalColor, alpha);
}
