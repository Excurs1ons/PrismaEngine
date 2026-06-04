#version 460 core
// ============================================================
// deferred_lighting.frag — PrismaEngine 延迟渲染光照片元着色器
//
// 从 G-buffer 读取几何数据，计算光照:
//   - PBR Metallic-Roughness 光照 (Cook-Torrance BRDF)
//   - 方向光/点光源 (最多 8 个)
//   - IBL (辐照度贴图 + 预过滤贴图 + BRDF LUT)
//   - 阴影贴图采样
// ============================================================

const float PBR_PI = 3.14159265359;
const float PBR_EPSILON = 0.0001;
const int MAX_DEFERRED_LIGHTS = 8;
const int MAX_CASCADES = 4;

// ---- 片元输入 (来自 deferred_fullscreen.vert) ----
layout(location = 0) in vec2 v_TexCoord;

layout(location = 0) out vec4 outColor;

// ---- 光源结构 (匹配 C++ Light struct) ----
struct DeferredLight {
    vec4 position;  // xyz: position, w: type (0=dir, 1=point, 2=spot)
    vec4 color;     // rgb: color * intensity, w: range
    vec4 direction; // xyz: direction, w: spotAngle
};

// ==================== Set 0: G-buffer 输入 ====================
// 这 4 个纹理对应 GBuffer 的 4 个颜色 RT

layout(set = 0, binding = 0) uniform sampler2D GBufferPosition;  // RGB: pos, A: roughness
layout(set = 0, binding = 1) uniform sampler2D GBufferNormal;    // RGB: normal, A: metallic
layout(set = 0, binding = 2) uniform sampler2D GBufferAlbedo;    // RGB: albedo, A: ao
layout(set = 0, binding = 3) uniform sampler2D GBufferEmissive;  // RGB: emissive, A: matID
layout(set = 0, binding = 4) uniform sampler2D GBufferDepth;     // R: depth

// ==================== Set 1: 场景数据 ====================

layout(set = 1, binding = 0, std140) uniform SceneData {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
    vec4 cameraPos;
} scene;

// ==================== Set 2: 光源数据 ====================

layout(set = 2, binding = 0, std430) readonly buffer DeferredLightBuffer {
    DeferredLight lights[];
} lightBuffer;

layout(set = 2, binding = 1, std140) uniform LightCount {
    uint numLights;
} lightCount;

// ==================== Set 3: 阴影贴图 ====================

layout(set = 3, binding = 0) uniform sampler2DShadow ShadowMapArray[MAX_CASCADES];

// ==================== Set 4: IBL 环境光照 ====================

layout(set = 4, binding = 0) uniform samplerCube IrradianceMap;
layout(set = 4, binding = 1) uniform samplerCube PrefilterMap;
layout(set = 4, binding = 2) uniform sampler2D BRDFLUT;

// ===================== Push Constants =====================

layout(push_constant) uniform LightingPushConstants {
    vec4 ambient;       // rgb: ambient color, w: padding
    vec4 lightDir;      // xyz: main light direction, w: padding
    vec4 lightColor;    // rgb: main light color, w: padding
    vec4 mainLightPos;  // xyz: main light pos, w: range
} pc;

// ===================== 从深度重建世界位置 =====================

vec3 ReconstructWorldPos(vec2 uv, float depth) {
    // 将 UV 和深度转换到裁剪空间
    vec4 clipPos = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewPos = inverse(scene.projection) * clipPos;
    viewPos /= viewPos.w;
    vec4 worldPos = inverse(scene.view) * viewPos;
    return worldPos.xyz;
}

// ===================== PBR 核心函数 (同 pbr_lit.frag) =====================

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
    float denom = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + PBR_EPSILON;
    return num / denom;
}

vec3 CalculatePBRLight(vec3 albedo, float metallic, float roughness,
                        vec3 N, vec3 V, vec3 L, vec3 radiance,
                        float shadowFactor) {
    vec3 H = normalize(V + L);

    vec3 f0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = FresnelSchlick(f0, max(dot(H, V), 0.0));
    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);

    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = kD * albedo / PBR_PI;
    vec3 specular = SpecularBRDF(N, V, L, H, roughness, f0);

    return (diffuse + specular) * radiance * NdotL * shadowFactor;
}

vec3 SampleIBLDiffuse(vec3 N, vec3 V, vec3 F0, float roughness, float NdotV) {
    vec3 kS = FresnelSchlickRoughness(F0, NdotV, roughness);
    vec3 kD = (1.0 - kS) * (1.0 - /*metallic*/ 0.0); // metallic 已经在外部处理

    vec3 irradiance = texture(IrradianceMap, N).rgb;
    vec3 prefilteredColor = textureLod(PrefilterMap, reflect(-V, N), roughness * 4.0).rgb;
    vec2 brdf = texture(BRDFLUT, vec2(NdotV, roughness)).rg;
    vec3 specular = prefilteredColor * (F0 * brdf.x + brdf.y);

    return kD * irradiance + specular;
}

// ===================== 阴影采样 =====================

float SampleShadowMap(sampler2DShadow shadowMap, vec3 projCoords, float bias) {
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
        vec2 offset = poissonDisk[i] * 0.002;
        shadow += texture(shadowMap, vec3(projCoords.xy + offset, projCoords.z - bias));
    }
    return shadow / float(samples);
}

float CascadeShadow(vec3 worldPos, float cascadeCount) {
    const float cascadeDistances[4] = float[](25.0, 60.0, 150.0, 400.0);
    float dist = length(scene.cameraPos.xyz - worldPos);

    int cascadeIndex = 0;
    for (int i = 0; i < int(cascadeCount); i++) {
        if (dist > cascadeDistances[i]) {
            cascadeIndex = i + 1;
        }
    }
    cascadeIndex = min(cascadeIndex, int(cascadeCount) - 1);

    vec4 fragPosLightSpace = scene.viewProjection * vec4(worldPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0) return 1.0;

    return SampleShadowMap(ShadowMapArray[cascadeIndex], projCoords, 0.005);
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
    // ===== 1. 从 G-buffer 采样 =====
    vec4 positionRoughness = texture(GBufferPosition, v_TexCoord);
    vec4 normalMetallic     = texture(GBufferNormal, v_TexCoord);
    vec4 albedoAO           = texture(GBufferAlbedo, v_TexCoord);
    vec4 emissiveMatID      = texture(GBufferEmissive, v_TexCoord);
    float depth             = texture(GBufferDepth, v_TexCoord).r;

    // 解码 G-buffer 数据
    vec3 worldPos  = positionRoughness.xyz;
    float roughness = positionRoughness.w;
    vec3 N = normalize(normalMetallic.xyz * 2.0 - 1.0);
    float metallic = normalMetallic.w;
    vec3 albedo   = albedoAO.rgb;
    float ao      = albedoAO.a;
    vec3 emissive = emissiveMatID.rgb;
    uint matID    = uint(emissiveMatID.a + 0.5);

    // ===== 2. 视角方向 =====
    vec3 V = normalize(scene.cameraPos.xyz - worldPos);

    // ===== 3. 直接光照 =====
    float shadowFactor = CascadeShadow(worldPos, 4.0);
    vec3 directLighting = vec3(0.0);

    // 从 push constants 读取主方向光
    {
        vec3 L = normalize(-pc.lightDir.xyz);
        vec3 radiance = pc.lightColor.rgb;
        directLighting += CalculatePBRLight(albedo, metallic, roughness, N, V, L, radiance, shadowFactor);
    }

    // 遍历额外光源 (SSBO)
    uint numLights = min(lightCount.numLights, uint(MAX_DEFERRED_LIGHTS));
    for (uint i = 0; i < numLights; i++) {
        DeferredLight light = lightBuffer.lights[i];

        int type = int(light.position.w + 0.5);
        vec3 L;
        vec3 radiance = light.color.rgb;

        if (type == 0) {
            // 方向光
            L = normalize(-light.direction.xyz);
            {
                // 应用阴影
                float lightShadow = shadowFactor;
                directLighting += CalculatePBRLight(albedo, metallic, roughness, N, V, L, radiance, lightShadow);
            }
        } else {
            // 点光源
            L = normalize(light.position.xyz - worldPos);
            float dist = length(light.position.xyz - worldPos);
            float range = light.color.w;
            if (range > 0.0 && dist < range) {
                float atten = max(1.0 - (dist * dist) / (range * range), 0.0);
                radiance *= atten * atten;
                directLighting += CalculatePBRLight(albedo, metallic, roughness, N, V, L, radiance, 1.0);
            }
        }
    }

    // ===== 4. IBL 环境光照 =====
    float NdotV = max(dot(N, V), 0.0);
    vec3 f0 = mix(vec3(0.04), albedo, metallic);
    vec3 ibl = SampleIBLDiffuse(N, V, f0, roughness, NdotV) * ao;

    // ===== 5. 合成 =====
    vec3 finalColor = directLighting + ibl + emissive;

    // ===== 6. 色调映射 =====
    finalColor = ACESToneMapping(finalColor, 1.0);
    finalColor = GammaCorrection(finalColor, 2.2);

    outColor = vec4(finalColor, 1.0);
}
