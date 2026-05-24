#pragma once
// 默认着色器硬编码 - 避免外部文件依赖

namespace Prisma {
namespace Graphic {

// 默认顶点着色器 (HLSL)
inline const char* DEFAULT_VERTEX_SHADER = R"(
cbuffer ViewProjectionBuffer : register(b0)
{
    matrix ViewProjection;
}

cbuffer WorldBuffer : register(b1)
{
    matrix World;
}

cbuffer BaseColorBuffer : register(b2)
{
    float4 BaseColor;
}

cbuffer MaterialParamsBuffer : register(b3)
{
    float Metallic;
    float Roughness;
    float Emissive;
    float NormalScale;
}

struct VS_IN
{
    float3 pos : POSITION;
    float4 col : COLOR;
};

struct PS_IN
{
    float4 pos : SV_POSITION;
    float4 col : COLOR;
};

PS_IN VSMain(VS_IN input)
{
    PS_IN output;

    // 计算世界空间位置
    float4 worldPos = mul(float4(input.pos, 1.0), World);

    // 调试：直接在世界空间位置加上偏移，确保可见
    // 在屏幕中心显示一个小三角形
    output.pos = float4(
        worldPos.x * 0.1,    // 缩小x坐标
        worldPos.y * 0.1,    // 缩小y坐标
        0.5,                // 固定z值
        1.0                 // w值
    );

    // 使用顶点颜色和基础颜色的混合
    output.col = input.col * BaseColor;

    return output;
}
)";

// 默认像素着色器 (HLSL)
inline const char* DEFAULT_PIXEL_SHADER = R"(
cbuffer MaterialParamsBuffer : register(b3)
{
    float Metallic;
    float Roughness;
    float Emissive;
    float NormalScale;
}

struct PS_IN
{
    float4 pos : SV_POSITION;
    float4 col : COLOR;
};

float4 PSMain(PS_IN input) : SV_TARGET
{
    // 简单地返回输入颜色
    // 添加自发光效果
    float3 emissiveColor = input.col.rgb * Emissive;
    return float4(input.col.rgb + emissiveColor, input.col.a);
}
)";

// 清屏用着色器 - 仅顶点着色器
inline const char* CLEAR_VERTEX_SHADER = R"(
struct VS_IN
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
};

struct PS_IN
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

PS_IN VSMain(VS_IN input)
{
    PS_IN output;
    output.pos = float4(input.pos, 1.0);
    output.uv = input.uv;
    return output;
}
)";

// 清屏用着色器 - 像素着色器
inline const char* CLEAR_PIXEL_SHADER = R"(
cbuffer ClearColorBuffer : register(b0)
{
    float4 ClearColor;
};

struct PS_IN
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

float4 PSMain(PS_IN input) : SV_TARGET
{
    return ClearColor;
}
)";

// 天空盒顶点着色器 (HLSL)
inline const char* SKYBOX_VERTEX_SHADER = R"(
cbuffer ConstantBuffer : register(b0)
{
    float4x4 mViewProjection;
};

struct VS_INPUT
{
    float3 position : POSITION;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float3 texCoord : TEXCOORD0;
};

// 顶点着色器入口点
PS_INPUT VSMain(VS_INPUT input)
{
    PS_INPUT output;

    // 将顶点位置转换为齐次坐标
    float4 pos = float4(input.position, 1.0f);

    // 应用视图投影矩阵
    // 注意：对于天空盒，我们通常会移除视图矩阵中的平移部分
    // 这样天空盒就会跟随摄像机移动，看起来永远无法到达
    float4x4 viewProjection = mViewProjection;
    viewProjection._m30 = 0.0f;
    viewProjection._m31 = 0.0f;
    viewProjection._m32 = 0.0f;

    // 将w分量设置为z分量，这样可以确保天空盒总是在最远的地方渲染
    output.position = mul(pos, viewProjection);
    output.position.z = output.position.w;

    // 纹理坐标就是顶点位置本身，因为我们要从立方体贴图中采样
    output.texCoord = input.position;

    return output;
}
)";

// 天空盒像素着色器 (HLSL)
inline const char* SKYBOX_PIXEL_SHADER = R"(
#ifdef USE_TEXTURE
TextureCube skyboxTexture : register(t0);
SamplerState skyboxSampler : register(s0);
#endif

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float3 texCoord : TEXCOORD0;
};

// 像素着色器入口点
float4 PSMain(PS_INPUT input) : SV_TARGET
{
#ifdef USE_TEXTURE
    // 从立方体贴图中采样颜色
    float4 color = skyboxTexture.Sample(skyboxSampler, input.texCoord);
    return color;
#else
    // 如果没有定义纹理，则返回纯色（粉红色）便于识别
    return float4(1.0f, 0.0f, 1.0f, 1.0f); // 粉红色
#endif
}
)";

// ========== 延迟渲染着色器 ==========

// 几何通道顶点着色器 (HLSL)
inline const char* DEFERRED_GEOMETRY_VERTEX_SHADER = R"(
cbuffer ViewProjectionBuffer : register(b0)
{
    matrix ViewProjection;
}

cbuffer WorldBuffer : register(b1)
{
    matrix World;
    matrix WorldInverseTranspose;
}

cbuffer MaterialBuffer : register(b2)
{
    float4 BaseColor;
    float Metallic;
    float Roughness;
    float Emissive;
    float AO;
    uint MaterialID;
}

struct VS_IN
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float4 color : COLOR;
};

struct PS_IN
{
    float4 pos : SV_POSITION;
    float3 worldPos : POSITION1;
    float3 worldNormal : NORMAL;
    float2 uv : TEXCOORD0;
    float4 color : COLOR;
    uint materialID : TEXCOORD1;
};

PS_IN VSMain(VS_IN input)
{
    PS_IN output;

    // 计算世界空间位置
    float4 worldPos = mul(float4(input.pos, 1.0), World);
    output.worldPos = worldPos.xyz;

    // 计算裁剪空间位置
    output.pos = mul(worldPos, ViewProjection);

    // 计算世界空间法线
    output.worldNormal = normalize(mul(input.normal, (float3x3)WorldInverseTranspose));

    // 传递其他数据
    output.uv = input.uv;
    output.color = input.color * BaseColor;
    output.materialID = MaterialID;

    return output;
}
)";

// 几何通道像素着色器 (HLSL) - 输出到多个渲染目标
inline const char* DEFERRED_GEOMETRY_PIXEL_SHADER = R"(
cbuffer MaterialBuffer : register(b2)
{
    float4 BaseColor;
    float Metallic;
    float Roughness;
    float Emissive;
    float AO;
    uint MaterialID;
}

struct PS_IN
{
    float4 pos : SV_POSITION;
    float3 worldPos : POSITION1;
    float3 worldNormal : NORMAL;
    float2 uv : TEXCOORD0;
    float4 color : COLOR;
    uint materialID : TEXCOORD1;
};

struct GBufferOutput
{
    float4 position : SV_TARGET0;  // Position (RGB) + Roughness (A)
    float4 normal : SV_TARGET1;    // Normal (RGB) + Metallic (A)
    float4 albedo : SV_TARGET2;    // Albedo (RGB) + AO (A)
    float4 emissive : SV_TARGET3;  // Emissive (RGB) + MaterialID (A)
};

GBufferOutput PSMain(PS_IN input)
{
    GBufferOutput output;

    // 位置 + 粗糙度
    output.position = float4(input.worldPos, Roughness);

    // 法线 + 金属度 (编码法线到[0,1]范围)
    float3 encodedNormal = input.worldNormal * 0.5 + 0.5;
    output.normal = float4(encodedNormal, Metallic);

    // 颜色 + AO
    output.albedo = float4(input.color.rgb, AO);

    // 自发光 + 材质ID
    float emissiveStrength = Emissive;
    output.emissive = float4(input.color.rgb * emissiveStrength, asfloat(input.materialID));

    return output;
}
)";

// 光照通道顶点着色器 (HLSL) - 全屏四边形
inline const char* DEFERRED_LIGHTING_VERTEX_SHADER = R"(
struct VS_IN
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD0;
};

struct PS_IN
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

PS_IN VSMain(VS_IN input)
{
    PS_IN output;
    output.pos = float4(input.pos, 1.0);
    output.uv = input.uv;
    return output;
}
)";

// 光照通道像素着色器 (HLSL) - PBR光照计算
inline const char* DEFERRED_LIGHTING_PIXEL_SHADER = R"(
// G-Buffer纹理
Texture2D GBufferPosition : register(t0);
Texture2D GBufferNormal : register(t1);
Texture2D GBufferAlbedo : register(t2);
Texture2D GBufferEmissive : register(t3);
Texture2D GBufferDepth : register(t4);

// 阴影贴图 (可选)
Texture2D ShadowMap : register(t5);
SamplerState ShadowSampler : register(s1);

// 采样器
SamplerState GBufferSampler : register(s0);

// 相机参数
cbuffer CameraBuffer : register(b0)
{
    float3 CameraPosition;
    float padding1;
    matrix InverseViewProjection;
}

// 光照参数
cbuffer LightBuffer : register(b1)
{
    float3 LightDirection;
    float LightType; // 0=directional, 1=point, 2=spot
    float3 LightColor;
    float LightIntensity;
    float3 LightPosition;
    float LightRadius;
    float3 LightAttenuation;
    float padding2;
    matrix LightViewProjection; // 用于阴影贴图
}

// 环境光参数
cbuffer AmbientBuffer : register(b2)
{
    float3 AmbientColor;
    float AmbientIntensity;
}

struct PS_IN
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

// 法线解码函数
float3 DecodeNormal(float3 encodedNormal)
{
    return encodedNormal * 2.0 - 1.0;
}

// 计算PBR BRDF
float3 CalculatePBR(float3 albedo, float metallic, float roughness, float3 normal, float3 viewDir, float3 lightDir, float3 lightColor)
{
    float3 N = normalize(normal);
    float3 V = normalize(viewDir);
    float3 L = normalize(lightDir);
    float3 H = normalize(V + L);

    // 能量保存的漫反射
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);

    // Cook-Torrance BRDF
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float HdotV = max(dot(H, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);

    // D - 微表面分布 (GGX)
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float denom = NdotH * NdotH * (alpha2 - 1.0) + 1.0;
    float D = alpha2 / (3.14159265 * denom * denom);

    // F - 菲涅尔 (Schlick近似)
    float3 F = F0 + (1.0 - F0) * pow(1.0 - HdotV, 5.0);

    // G - 几何遮蔽 (Smith)
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    float G1L = NdotL / (NdotL * (1.0 - k) + k);
    float G1V = NdotV / (NdotV * (1.0 - k) + k);
    float G = G1L * G1V;

    // BRDF
    float3 numerator = D * F * G;
    float denominator = 4.0 * NdotV * NdotL + 0.001;
    float3 specular = numerator / denominator;

    // 漫反射和镜面反射的能量保存
    float3 kS = F;
    float3 kD = (1.0 - kS) * (1.0 - metallic);

    float3 diffuse = kD * albedo / 3.14159265;

    return (diffuse + specular) * lightColor * NdotL;
}

float4 PSMain(PS_IN input) : SV_TARGET
{
    // 从G-Buffer采样数据
    float4 positionData = GBufferPosition.Sample(GBufferSampler, input.uv);
    float4 normalData = GBufferNormal.Sample(GBufferSampler, input.uv);
    float4 albedoData = GBufferAlbedo.Sample(GBufferSampler, input.uv);
    float4 emissiveData = GBufferEmissive.Sample(GBufferSampler, input.uv);

    // 跳过天空盒像素
    if (positionData.w < 0.01) {
        return float4(0, 0, 0, 1);
    }

    // 解码G-Buffer数据
    float3 worldPos = positionData.rgb;
    float roughness = positionData.w;
    float3 worldNormal = DecodeNormal(normalData.rgb);
    float metallic = normalData.w;
    float3 albedo = albedoData.rgb;
    float ao = albedoData.w;
    float3 emissive = emissiveData.rgb;

    // 计算视线方向
    float3 viewDir = normalize(CameraPosition - worldPos);

    // 计算光照
    float3 finalColor = float3(0, 0, 0);

    // 环境光
    finalColor += AmbientColor * AmbientIntensity * albedo * ao;

    // 主光源
    if (LightType == 0) { // 方向光
        float3 lightDir = normalize(-LightDirection);
        float3 lightColor = LightColor * LightIntensity;
        finalColor += CalculatePBR(albedo, metallic, roughness, worldNormal, viewDir, lightDir, lightColor);
    }
    else if (LightType == 1) { // 点光源
        float3 lightDir = LightPosition - worldPos;
        float distance = length(lightDir);

        if (distance < LightRadius) {
            lightDir = normalize(lightDir);
            float attenuation = 1.0 / (LightAttenuation.x + LightAttenuation.y * distance + LightAttenuation.z * distance * distance);
            float3 lightColor = LightColor * LightIntensity * attenuation;
            finalColor += CalculatePBR(albedo, metallic, roughness, worldNormal, viewDir, lightDir, lightColor);
        }
    }

    // 添加自发光
    finalColor += emissive;

    return float4(finalColor, 1.0);
}
)";

// 合成通道顶点着色器 (HLSL)
inline const char* DEFERRED_COMPOSITION_VERTEX_SHADER = R"(
struct VS_IN
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD0;
};

struct PS_IN
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

PS_IN VSMain(VS_IN input)
{
    PS_IN output;
    output.pos = float4(input.pos, 1.0);
    output.uv = input.uv;
    return output;
}
)";

// 合成通道像素着色器 (HLSL)
inline const char* DEFERRED_COMPOSITION_PIXEL_SHADER = R"(
Texture2D LightingResult : register(t0);
Texture2D SkyboxResult : register(t1);
Texture2D TransparentResult : register(t2);
Texture2D DepthBuffer : register(t3);
SamplerState ScreenSampler : register(s0);

cbuffer PostProcessBuffer : register(b0)
{
    float4 ToneMappingParams; // x=exposure, y=unused, z=unused, w=unused
    float4 GammaParams;       // x=gamma, y=unused, z=unused, w=unused
    uint EnableToneMapping;
    uint EnableGammaCorrection;
    uint EnableBloom;
    float padding;
}

struct PS_IN
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

// ACES色调映射
float3 ACESToneMapping(float3 color, float exposure)
{
    color *= exposure;
    const float A = 2.51;
    const float B = 0.03;
    const float C = 2.43;
    const float D = 0.59;
    const float E = 0.14;
    return (color * (A * color + B)) / (color * (C * color + D) + E);
}

float4 PSMain(PS_IN input) : SV_TARGET
{
    // 采样深度来决定哪些像素使用天空盒
    float depth = DepthBuffer.Sample(ScreenSampler, input.uv).r;

    float3 finalColor;

    if (depth >= 1.0) {
        // 天空盒像素
        finalColor = SkyboxResult.Sample(ScreenSampler, input.uv).rgb;
    } else {
        // 场景像素
        finalColor = LightingResult.Sample(ScreenSampler, input.uv).rgb;

        // 叠加透明物体
        float4 transparentColor = TransparentResult.Sample(ScreenSampler, input.uv);
        finalColor = finalColor * (1.0 - transparentColor.a) + transparentColor.rgb * transparentColor.a;
    }

    // 后处理效果

    // 色调映射
    if (EnableToneMapping) {
        finalColor = ACESToneMapping(finalColor, ToneMappingParams.x);
    }

    // 伽马校正
    if (EnableGammaCorrection) {
        finalColor = pow(finalColor, 1.0 / GammaParams.x);
    }

    return float4(finalColor, 1.0);
}
)";

// ========== PBR 前向渲染着色器 (HLSL) ==========

// PBR 前向顶点着色器 (HLSL)
inline const char* PBR_FORWARD_VERTEX_SHADER = R"(
cbuffer ViewProjectionBuffer : register(b0)
{
    matrix ViewProjection;
    matrix View;
    matrix Projection;
    float4 CameraPos;
}

cbuffer WorldBuffer : register(b1)
{
    matrix World;
    matrix WorldInverseTranspose;
}

cbuffer MaterialBuffer : register(b2)
{
    float4 BaseColor;
    float Metallic;
    float Roughness;
    float AO;
    float EmissiveIntensity;
    float padding[2];
}

struct VS_IN
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float4 color : COLOR;
    float3 tangent : TANGENT;
};

struct PS_IN
{
    float4 pos : SV_POSITION;
    float3 worldPos : POSITION1;
    float3 worldNormal : NORMAL;
    float2 uv : TEXCOORD0;
    float4 color : COLOR;
    float3 worldTangent : TANGENT;
    float3 viewDir : TEXCOORD1;
};

PS_IN VSMain(VS_IN input)
{
    PS_IN output;

    float4 worldPos = mul(float4(input.pos, 1.0), World);
    output.worldPos = worldPos.xyz;
    output.pos = mul(worldPos, ViewProjection);

    output.worldNormal = normalize(mul(input.normal, (float3x3)WorldInverseTranspose));

    if (length(input.tangent) > 0.001) {
        output.worldTangent = normalize(mul(input.tangent, (float3x3)WorldInverseTranspose));
    } else {
        output.worldTangent = float3(0, 0, 0);
    }

    output.uv = input.uv;
    output.color = input.color * BaseColor;
    output.viewDir = CameraPos.xyz - worldPos.xyz;

    return output;
}
)";

// PBR 前向像素着色器 (HLSL) - 完整 PBR + IBL
inline const char* PBR_FORWARD_PIXEL_SHADER = R"(
// 纹理采样器
Texture2D AlbedoMap : register(t0);
Texture2D NormalMap : register(t1);
Texture2D MetallicRoughnessMap : register(t2);
Texture2D AOMap : register(t3);
Texture2D EmissiveMap : register(t4);

// IBL 环境贴图
TextureCube IrradianceMap : register(t5);
TextureCube PrefilterMap : register(t6);
Texture2D BRDFLUT : register(t7);

SamplerState DefaultSampler : register(s0);

cbuffer MaterialBuffer : register(b2)
{
    float4 BaseColor;
    float Metallic;
    float Roughness;
    float AO;
    float EmissiveIntensity;
    float padding[2];
}

cbuffer LightBuffer : register(b3)
{
    float3 LightDirection;
    float LightType;
    float3 LightColor;
    float LightIntensity;
    float3 LightPosition;
    float LightRange;
    float3 LightAttenuation;
    float numLights;
    float Exposure;
    float Gamma;
    float EnvIntensity;
    matrix LightViewProjection;
}

cbuffer CameraBuffer : register(b4)
{
    matrix ViewProjection;
    matrix View;
    matrix Projection;
    float3 CameraPos;
}

struct PS_IN
{
    float4 pos : SV_POSITION;
    float3 worldPos : POSITION1;
    float3 worldNormal : NORMAL;
    float2 uv : TEXCOORD0;
    float4 color : COLOR;
    float3 worldTangent : TANGENT;
    float3 viewDir : TEXCOORD1;
};

static const float PI = 3.14159265;
static const float EPSILON = 0.0001;

float3 FresnelSchlick(float3 f0, float cosTheta)
{
    return f0 + (1.0 - f0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

float3 FresnelSchlickRoughness(float3 f0, float cosTheta, float roughness)
{
    return f0 + (max(float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), f0) - f0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float alpha = max(roughness * roughness, EPSILON);
    float alpha2 = alpha * alpha;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = NdotH2 * (alpha2 - 1.0) + 1.0;
    denom = max(PI * denom * denom, EPSILON);
    return alpha2 / denom;
}

float SchlickGGX(float NdotV, float roughness)
{
    float r = max(roughness, EPSILON);
    float k = (r * r) / 2.0;
    float denom = NdotV * (1.0 - k) + k;
    return NdotV / max(denom, EPSILON);
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return SchlickGGX(NdotV, roughness) * SchlickGGX(NdotL, roughness);
}

float3 SpecularBRDF(float3 N, float3 V, float3 L, float3 H, float roughness, float3 f0)
{
    float D = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    float3 F = FresnelSchlick(f0, max(dot(H, V), 0.0));
    float3 num = D * G * F;
    float den = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + EPSILON;
    return num / den;
}

float3 CalculatePBRLight(float3 albedo, float metallic, float roughness,
                         float3 N, float3 V, float3 L, float3 radiance)
{
    float3 H = normalize(V + L);
    float3 f0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);
    float3 F = FresnelSchlick(f0, max(dot(H, V), 0.0));
    float3 kS = F;
    float3 kD = (1.0 - kS) * (1.0 - metallic);
    float NdotL = max(dot(N, L), 0.0);
    float3 diffuse = kD * albedo / PI;
    float3 specular = SpecularBRDF(N, V, L, H, roughness, f0);
    return (diffuse + specular) * radiance * NdotL;
}

float3 ACESToneMapping(float3 color, float exposure)
{
    color *= exposure;
    const float A = 2.51;
    const float B = 0.03;
    const float C = 2.43;
    const float D = 0.59;
    const float E = 0.14;
    return (color * (A * color + B)) / (color * (C * color + D) + E);
}

float4 PSMain(PS_IN input) : SV_TARGET
{
    // 采样材质贴图
    float4 albedoSample = AlbedoMap.Sample(DefaultSampler, input.uv);
    float3 albedo = BaseColor.rgb * albedoSample.rgb * input.color.rgb;

    float metallic = Metallic;
    float roughness = Roughness;
    if (MetallicRoughnessMap.CalculateLevelOfDetail(DefaultSampler, 0).x > 1) {
        float4 mrSample = MetallicRoughnessMap.Sample(DefaultSampler, input.uv);
        metallic *= mrSample.b;
        roughness *= mrSample.g;
    }

    float ao = AO;
    if (AOMap.CalculateLevelOfDetail(DefaultSampler, 0).x > 1) {
        ao *= AOMap.Sample(DefaultSampler, input.uv).r;
    }

    float3 emissive = float3(0, 0, 0);
    if (EmissiveIntensity > 0.001) {
        if (EmissiveMap.CalculateLevelOfDetail(DefaultSampler, 0).x > 1) {
            emissive = EmissiveMap.Sample(DefaultSampler, input.uv).rgb * EmissiveIntensity;
        } else {
            emissive = albedo * EmissiveIntensity;
        }
    }

    // 法线贴图
    float3 N = normalize(input.worldNormal);
    if (length(input.worldTangent) > 0.001 && NormalMap.CalculateLevelOfDetail(DefaultSampler, 0).x > 1) {
        float3 T = normalize(input.worldTangent);
        float3 B = cross(N, T);
        float3x3 TBN = float3x3(T, B, N);
        float3 normalTS = NormalMap.Sample(DefaultSampler, input.uv).rgb * 2.0 - 1.0;
        N = normalize(mul(normalTS, TBN));
    }

    float3 V = normalize(input.viewDir);

    // 直接光照
    float3 directLighting = float3(0, 0, 0);

    // 方向光/主光源
    if (length(LightColor) > 0.001) {
        float3 L = normalize(-LightDirection);
        float3 radiance = LightColor * LightIntensity;
        directLighting += CalculatePBRLight(albedo, metallic, roughness, N, V, L, radiance);
    }

    // IBL 环境光照
    float3 ambientLighting = float3(0, 0, 0);
    float3 f0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);

    // 漫反射 IBL
    float texWidth, texHeight, texLevels;
    IrradianceMap.GetDimensions(0, texWidth, texHeight, texLevels);
    if (texWidth > 1) {
        float3 irradiance = IrradianceMap.Sample(DefaultSampler, N).rgb * EnvIntensity;
        float NdotV = max(dot(N, V), 0.0);
        float3 kD = (1.0 - FresnelSchlickRoughness(f0, NdotV, roughness)) * (1.0 - metallic);
        ambientLighting += kD * albedo / PI * irradiance * ao;
    }

    // 镜面反射 IBL
    PrefilterMap.GetDimensions(0, texWidth, texHeight, texLevels);
    BRDFLUT.GetDimensions(0, texWidth, texHeight, texLevels);
    if (texWidth > 1) {
        float3 R = reflect(-V, N);
        float lod = roughness * 4.0;
        float3 prefilteredColor = PrefilterMap.SampleLevel(DefaultSampler, R, lod).rgb * EnvIntensity;
        float2 envBRDF = BRDFLUT.Sample(DefaultSampler, float2(max(dot(N, V), 0.0), roughness)).rg;
        float3 F_ibl = FresnelSchlickRoughness(f0, max(dot(N, V), 0.0), roughness);
        ambientLighting += prefilteredColor * (F_ibl * envBRDF.x + envBRDF.y);
    }

    // 环境光兜底
    if (ambientLighting.r < 0.001 && ambientLighting.g < 0.001 && ambientLighting.b < 0.001) {
        ambientLighting = albedo * 0.03 * ao;
    }

    // 合成
    float3 finalColor = directLighting + ambientLighting + emissive;

    // 后处理
    finalColor = ACESToneMapping(finalColor, Exposure);
    finalColor = pow(max(finalColor, float3(0, 0, 0)), 1.0 / Gamma);

    return float4(finalColor, albedoSample.a * BaseColor.a);
}
)";

} // namespace Graphic
} // namespace Prisma