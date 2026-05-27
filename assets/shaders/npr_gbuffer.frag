// ============================================================
// npr_gbuffer.frag - PrismaEngine NPR G-Buffer 片元着色器
//
// 输出 3 个渲染目标:
//   RT0: 反照率 (RGB) + NdotV 占位 (A)   - RGBA8_UNorm
//   RT1: 世界法线 (RGB) + 金属标志 (A)    - RGBA16F
//   RT2: NPR 参数 (RGBA)                  - RGBA16F
//         R=rimPower, G=wrapAmount, B=shadowIntensity, A=emissiveIntensity
//
// 注意: NdotV 在延迟光照阶段计算 (需要 cameraPos)
// ============================================================

#version 450

// ---- 片元输入 ----
layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec4 inColor;
layout(location = 4) in vec3 inTangent;

// ---- G-Buffer 多渲染目标输出 ----
layout(location = 0) out vec4 outAlbedo;     // RGB: 反照率,  A: NdotV 占位
layout(location = 1) out vec4 outNormal;     // RGB: 世界法线, A: 金属标志
layout(location = 2) out vec4 outNPRParams;  // R: rimPower, G: wrapAmount, B: shadowIntensity, A: emissiveIntensity

// ---- NPR 材质数据 (Set 0) ----
layout(set = 0, binding = 0, std140) uniform NPRMaterialData {
    vec4 baseColor;         // 基础颜色 (rgba)
    vec4 rimColor;          // 边缘光颜色 (rgb, w=强度)
    vec4 shadowColor;       // 阴影着色 (rgb, w=强度)
    vec4 horizonColor;      // 暖色水平线 (rgb, w=强度)
    float roughness;        // 粗糙度 (映射到卡通高光尺寸)
    float rimPower;         // 边缘光宽度
    float wrapAmount;       // 漫反射包裹量 (SSS 模拟)
    float emissiveIntensity;// 自发光强度
} material;

layout(set = 0, binding = 1) uniform sampler2D albedoMap;
layout(set = 0, binding = 2) uniform sampler2D normalMap;

// ---- Push Constants ----
layout(push_constant) uniform PushConstants {
    mat4 world;
    vec4 color;
} pc;

// ---- Set 1: 场景数据 (相机) ----
layout(set = 1, binding = 0) uniform SceneData {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec4 cameraPos;
} scene;

void main() {
    // ===== 1. 反照率采样 =====
    vec4 albedoSample = texture(albedoMap, inUV);
    vec3 baseColor = inColor.rgb * pc.color.rgb * material.baseColor.rgb * albedoSample.rgb;

    // ===== 2. 法线贴图 (如果存在切线) =====
    vec3 N = normalize(inNormal);
    if (length(inTangent) > 0.001 && textureSize(normalMap, 0).x > 1) {
        vec3 T = normalize(inTangent);
        vec3 B = cross(N, T);
        mat3 TBN = mat3(T, B, N);
        vec3 normalTS = texture(normalMap, inUV).rgb * 2.0 - 1.0;
        N = normalize(TBN * normalTS);
    }

    // ===== 3. 计算 NdotV (用于 NPR 光照) =====
    vec3 V = normalize(scene.cameraPos.xyz - inWorldPos);
    float NdotV = max(dot(N, V), 0.0);

    // ===== 4. 输出 G-Buffer =====
    outAlbedo = vec4(baseColor, NdotV);

    // 法线编码到 [0,1]; A 保留
    outNormal = vec4(N * 0.5 + 0.5, 0.0);

    // NPR 参数: rimPower, wrapAmount, shadowIntensity (默认 0.5), emissiveIntensity
    outNPRParams = vec4(material.rimPower, material.wrapAmount, 0.5, material.emissiveIntensity);
}
