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

#ifdef USE_TEXTURE
TextureCube skyboxTexture : register(t0);
SamplerState skyboxSampler : register(s0);
#endif

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