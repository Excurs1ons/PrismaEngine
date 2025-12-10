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
    output.pos = float4(input.pos, 1.0);
    output.col = input.col;
    return output;
}

float4 PSMain(PS_IN input) : SV_TARGET
{
    return input.col;
}