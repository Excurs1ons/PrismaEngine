// 测试着色器 - 直接输出顶点位置和颜色
cbuffer ViewProjectionBuffer : register(b0)
{
    matrix ViewProjection;
};

cbuffer WorldBuffer : register(b1)
{
    matrix World;
};

cbuffer BaseColorBuffer : register(b2)
{
    float4 BaseColor;
};

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

float4 PSMain(PS_IN input) : SV_TARGET
{
    return input.col;
}