#pragma once
// 默认着色器硬编码 - 避免外部文件依赖

namespace Engine {
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

    // 应用世界矩阵和视图投影矩阵
    float4 worldPos = mul(float4(input.pos, 1.0), World);
    output.pos = mul(worldPos, ViewProjection);

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

} // namespace Graphic
} // namespace Engine