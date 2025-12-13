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

float4 PSMain(PS_IN input) : SV_TARGET
{
    return input.col;
}