// Tilemap HLSL Shader for DirectX 12

// Constant Buffer
cbuffer CameraBuffer : register(b0) {
    matrix u_ViewProjection;
    matrix u_View;
    matrix u_Projection;
};

// Texture arrays (using texture2d arrays)
Texture2D u_TilesetTextures[16] : register(t1);
SamplerState u_SamplerState : register(s0);

// Input vertex structure
struct VertexInput {
    float2 a_Position : POSITION;
    float2 a_TexCoord : TEXCOORD0;
    float a_TexIndex : TEXCOORD1;
    float4 a_Color : COLOR;
};

// Output to pixel shader
struct PixelInput {
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
    float TexIndex : TEXCOORD1;
    float4 Color : COLOR;
};

// Vertex Shader
PixelInput VSMain(VertexInput input) {
    PixelInput output;
    float4 worldPos = float4(input.a_Position, 0.0, 1.0);
    output.Position = mul(u_ViewProjection, worldPos);
    output.TexCoord = input.a_TexCoord;
    output.TexIndex = input.a_TexIndex;
    output.Color = input.a_Color;
    return output;
}

// Pixel Shader
float4 PSMain(PixelInput input) : SV_TARGET {
    int texIndex = (int)input.TexIndex;

    // Prevent out of bounds access
    if (texIndex < 0 || texIndex >= 16) {
        return input.Color;
    }

    // Sample from appropriate texture
    float4 texColor = u_TilesetTextures[texIndex].Sample(u_SamplerState, input.TexCoord);

    // Apply color modulation
    float4 finalColor = texColor * input.Color;

    // Alpha test (optional)
    if (finalColor.a < 0.01) {
        discard;
    }

    return finalColor;
}
