#version 460
// ============================================================================
// PrismaEngine — Terrain Fragment Shader
// 4 层纹理喷溅（高度+坡度混合）+ PBR 光照
// ============================================================================

const float PBR_PI = 3.14159265359;
const float PBR_EPSILON = 0.0001;

// ---- 片元输入 ----
layout(location = 0) in vec3 v_WorldPos;
layout(location = 1) in vec3 v_Normal;
layout(location = 2) in vec2 v_TexCoord;
layout(location = 3) in float v_Height;

layout(location = 0) out vec4 outColor;

// ---- 地形 Uniform 缓冲 (std140) ----
layout(std140, binding = 0) uniform TerrainUniforms {
    mat4  u_ViewProjection;
    vec4  u_CameraPos;        // xyz = position, w = padding
    vec4  u_BlendHeights;     // 4 层混合中心高度
    vec4  u_BlendWidths;      // 4 层混合过渡宽度
    vec4  u_SlopeFactors;     // 4 层坡度影响系数
    vec4  u_Tiling;           // x=tiling0-1, y=tiling2-3, zw=unused
    vec4  u_HeightRange;      // x=minHeight, y=maxHeight, z=invRange, w=unused
    vec4  u_PBRParams;        // x=roughness, y=metallic, z=AO, w=unused
    vec4  u_SunDirection;     // xyz=direction, w=unused
    vec4  u_SunColor;         // rgb=color*intensity, w=ambient intensity
} terrain;

// ---- 4 层纹理喷溅 ----
layout(binding = 1) uniform sampler2D u_SplatTexture0;  // 草
layout(binding = 2) uniform sampler2D u_SplatTexture1;  // 岩石
layout(binding = 3) uniform sampler2D u_SplatTexture2;  // 沙
layout(binding = 4) uniform sampler2D u_SplatTexture3;  // 雪

// ============== 纹理喷溅: 混合权重计算 ==============

// 计算单层的高度混合权重（类似 smoothstep）
float HeightBlendWeight(float height, float blendHeight, float blendWidth) {
    float halfWidth = blendWidth * 0.5;
    float low = blendHeight - halfWidth;
    float high = blendHeight + halfWidth;
    return clamp((height - low) / max(high - low, PBR_EPSILON), 0.0, 1.0);
}

// 计算单层的坡度影响
float SlopeBlendWeight(float slope, float slopeFactor) {
    return mix(1.0, 1.0 - slope, slopeFactor);
}

// 计算 4 层混合权重（基于高度和坡度）
vec4 ComputeBlendWeights(float height, float slope) {
    vec4 weights;
    weights.x = HeightBlendWeight(height, terrain.u_BlendHeights.x, terrain.u_BlendWidths.x)
              * SlopeBlendWeight(slope, terrain.u_SlopeFactors.x);
    weights.y = HeightBlendWeight(height, terrain.u_BlendHeights.y, terrain.u_BlendWidths.y)
              * SlopeBlendWeight(slope, terrain.u_SlopeFactors.y);
    weights.z = HeightBlendWeight(height, terrain.u_BlendHeights.z, terrain.u_BlendWidths.z)
              * SlopeBlendWeight(slope, terrain.u_SlopeFactors.z);
    weights.w = HeightBlendWeight(height, terrain.u_BlendHeights.w, terrain.u_BlendWidths.w)
              * SlopeBlendWeight(slope, terrain.u_SlopeFactors.w);

    // 归一化：所有权重之和约为 1.0
    float total = weights.x + weights.y + weights.z + weights.w;
    if (total > PBR_EPSILON) {
        weights /= total;
    } else {
        // 兜底：均匀分布
        weights = vec4(0.25);
    }
    return weights;
}

// 计算坡度（法线与垂直方向的夹角）
float ComputeSlope(vec3 normal) {
    return 1.0 - abs(normal.y); // 0 = 平坦, 1 = 垂直
}

// ============== PBR 光照函数 ==============

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

// Smith-Schlick 几何函数 (GGX)
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

    vec3 f0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = FresnelSchlick(f0, max(dot(H, V), 0.0));

    vec3 kS = F;
    vec3 kD = (1.0 - kS) * (1.0 - metallic);

    float NdotL = max(dot(N, L), 0.0);

    vec3 diffuse = kD * albedo / PBR_PI;
    vec3 specular = SpecularBRDF(N, V, L, H, roughness, f0);

    return (diffuse + specular) * radiance * NdotL;
}

// ============== 色调映射 ==============

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

// ============== 主函数 ==============

void main() {
    // ===== 1. 法线 =====
    vec3 N = normalize(v_Normal);

    // ===== 2. 计算混合权重 =====
    float slope = ComputeSlope(N);
    vec4 blendWeights = ComputeBlendWeights(v_Height, slope);

    // ===== 3. 纹理喷溅采样 =====
    // 每层使用各自的 UV 平铺
    vec2 uv01 = v_TexCoord * terrain.u_Tiling.x;  // 层 0,1 使用 tiling.x
    vec2 uv23 = v_TexCoord * terrain.u_Tiling.y;  // 层 2,3 使用 tiling.y

    vec4 color0 = texture(u_SplatTexture0, uv01);
    vec4 color1 = texture(u_SplatTexture1, uv01);
    vec4 color2 = texture(u_SplatTexture2, uv23);
    vec4 color3 = texture(u_SplatTexture3, uv23);

    // 混合最终反照率
    vec3 albedo = color0.rgb * blendWeights.x
                + color1.rgb * blendWeights.y
                + color2.rgb * blendWeights.z
                + color3.rgb * blendWeights.w;

    // ===== 4. PBR 参数 =====
    float roughness = terrain.u_PBRParams.x;
    float metallic  = terrain.u_PBRParams.y;
    float ao        = terrain.u_PBRParams.z;

    // ===== 5. 直接光照 =====
    vec3 V = normalize(terrain.u_CameraPos.xyz - v_WorldPos);
    vec3 L = normalize(-terrain.u_SunDirection.xyz);
    vec3 radiance = terrain.u_SunColor.rgb;

    vec3 directLighting = CalculatePBRLight(albedo, metallic, roughness, N, V, L, radiance);

    // ===== 6. 环境光照（简化半球环境光） =====
    float ambientIntensity = terrain.u_SunColor.w;
    vec3 ambient = albedo * ambientIntensity * ao;

    // ===== 7. 合成 =====
    vec3 finalColor = directLighting + ambient;

    // ===== 8. 后处理 =====
    finalColor = ACESToneMapping(finalColor, 1.0);
    finalColor = GammaCorrection(finalColor, 2.2);

    outColor = vec4(finalColor, 1.0);
}
