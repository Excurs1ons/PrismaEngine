// Text.hlsl - 文本渲染着色器

cbuffer ConstantBuffer : register(b0)
{
    float4x4 mProjection;
    float4   textColor;
};

struct VS_INPUT
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD0;
    float4 color    : COLOR;
};

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
    float4 color    : COLOR;
};

// 顶点着色器入口点
PS_INPUT VSMain(VS_INPUT input)
{
    PS_INPUT output;

    // 应用投影矩阵
    output.position = mul(float4(input.position, 1.0f), mProjection);
    output.texCoord = input.texCoord;
    output.color = input.color * textColor;

    return output;
}

// 字体纹理
Texture2D fontTexture : register(t0);
SamplerState fontSampler : register(s0);

// 像素着色器入口点
float4 PSMain(PS_INPUT input) : SV_TARGET
{
    // 从字体图集中采样
    float4 sampled = fontTexture.Sample(fontSampler, input.texCoord);

    // 使用 Alpha 通道进行混合
    // 字体纹理中 RGB 都是白色，只有 Alpha 通道有意义
    return float4(input.color.rgb, sampled.a * input.color.a);
}
