#include "ResourceManagerTypes.h"  // 包含完整类型定义
#include "ResourceManager.h"
#include "AssetSerializer.h"
#include "MeshAsset.h"
#include "TextureAsset.h"
#include <fstream>

namespace Engine {

void ResourceManager::CreateDefaultMeshes(const std::filesystem::path& meshesDir) {
    LOG_INFO("Resource", "创建默认网格资产...");
    // TODO: 暂时禁用，等待循环依赖问题完全解决
    LOG_INFO("Resource", "网格资产创建已暂时禁用");
}

void ResourceManager::CreateDefaultShaders(const std::filesystem::path& shadersDir) {
    LOG_INFO("Resource", "创建默认着色器资产...");

    try {
        // 创建基本顶点着色器
        std::ofstream basicVS(shadersDir / "BasicVS.hlsl");
        basicVS << R"(
struct VS_IN
{
    float3 pos : POSITION;
    float4 col : COLOR;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
};

struct PS_IN
{
    float4 pos : SV_POSITION;
    float4 col : COLOR;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
};

PS_IN VSMain(VS_IN input)
{
    PS_IN output;
    output.pos = float4(input.pos, 1.0);
    output.col = input.col;
    output.tex = input.tex;
    output.normal = input.normal;
    return output;
}
)";
        basicVS.close();

        // 创建基本像素着色器
        std::ofstream basicPS(shadersDir / "BasicPS.hlsl");
        basicPS << R"(
struct PS_IN
{
    float4 pos : SV_POSITION;
    float4 col : COLOR;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
};

Texture2D tex0 : register(t0);
SamplerState sampler0 : register(s0);

float4 PSMain(PS_IN input) : SV_TARGET
{
    return input.col;
}
)";
        basicPS.close();

        // 创建纹理像素着色器
        std::ofstream texturedPS(shadersDir / "TexturedPS.hlsl");
        texturedPS << R"(
struct PS_IN
{
    float4 pos : SV_POSITION;
    float4 col : COLOR;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
};

Texture2D tex0 : register(t0);
SamplerState sampler0 : register(s0);

float4 PSMain(PS_IN input) : SV_TARGET
{
    return tex0.Sample(sampler0, input.tex) * input.col;
}
)";
        texturedPS.close();

        // 创建PBR着色器
        std::ofstream pbrVS(shadersDir / "PBRVS.hlsl");
        pbrVS << R"(
cbuffer CameraBuffer : register(b0)
{
    matrix viewProj;
    matrix world;
    float3 cameraPos;
    float padding;
};

struct VS_IN
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD0;
    float3 tangent : TANGENT;
};

struct PS_IN
{
    float4 pos : SV_POSITION;
    float3 worldPos : POSITION;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD0;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

PS_IN VSMain(VS_IN input)
{
    PS_IN output;

    float4 worldPos = mul(float4(input.pos, 1.0), world);
    output.pos = mul(worldPos, viewProj);
    output.worldPos = worldPos.xyz;
    output.normal = mul(input.normal, (float3x3)world);
    output.tex = input.tex;
    output.tangent = mul(input.tangent, (float3x3)world);
    output.bitangent = cross(output.normal, output.tangent);

    return output;
}
)";
        pbrVS.close();

        std::ofstream pbrPS(shadersDir / "PBRPS.hlsl");
        pbrPS << R"(
cbuffer MaterialBuffer : register(b1)
{
    float3 albedo;
    float metallic;
    float roughness;
    float ao;
    float padding2;
};

cbuffer LightBuffer : register(b2)
{
    float3 lightPos;
    float3 lightColor;
    float3 viewPos;
};

Texture2D albedoMap : register(t0);
Texture2D normalMap : register(t1);
Texture2D metallicMap : register(t2);
Texture2D roughnessMap : register(t3);
Texture2D aoMap : register(t4);

SamplerState sampler0 : register(s0);

struct PS_IN
{
    float4 pos : SV_POSITION;
    float3 worldPos : POSITION;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD0;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

float3 getNormalFromMap(PS_IN input)
{
    float3 tangentNormal = normalMap.Sample(sampler0, input.tex).xyz * 2.0 - 1.0;

    float3 T = normalize(input.tangent);
    float3 B = normalize(input.bitangent);
    float3 N = normalize(input.normal);
    float3x3 TBN = float3x3(T, B, N);

    return normalize(mul(tangentNormal, TBN));
}

float distributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.14159265 * denom * denom;

    return num / denom;
}

float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float geometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometrySchlickGGX(NdotV, roughness);
    float ggx1 = geometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float4 PSMain(PS_IN input) : SV_TARGET
{
    float3 albedo = pow(albedoMap.Sample(sampler0, input.tex).rgb, float3(2.2, 2.2, 2.2));
    float metallic = metallicMap.Sample(sampler0, input.tex).r;
    float roughness = roughnessMap.Sample(sampler0, input.tex).r;
    float ao = aoMap.Sample(sampler0, input.tex).r;

    float3 N = getNormalFromMap(input);
    float3 V = normalize(viewPos - input.worldPos);
    float3 R = reflect(-V, N);

    // 计算反射率
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);

    // 光照方程
    float3 Lo = float3(0.0, 0.0, 0.0);

    // 计算每个光源的贡献
    float3 L = normalize(lightPos - input.worldPos);
    float3 H = normalize(V + L);
    float distance = length(lightPos - input.worldPos);
    float attenuation = 1.0 / (distance * distance);
    float3 radiance = lightColor * attenuation;

    // Cook-Torrance BRDF
    float NDF = distributionGGX(N, H, roughness);
    float G = geometrySmith(N, V, L, roughness);
    float3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    float3 kS = F;
    float3 kD = float3(1.0, 1.0, 1.0) - kS;
    kD *= 1.0 - metallic;

    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    float3 specular = numerator / denominator;

    float NdotL = max(dot(N, L), 0.0);
    Lo += (kD * albedo / 3.14159265 + specular) * radiance * NdotL;

    // 环境光
    float3 ambient = float3(0.03, 0.03, 0.03) * albedo * ao;

    float3 color = ambient + Lo;

    // HDR色调映射
    color = color / (color + float3(1.0, 1.0, 1.0));
    // Gamma校正
    color = pow(color, float3(1.0/2.2, 1.0/2.2, 1.0/2.2));

    return float4(color, 1.0);
}
)";
        pbrPS.close();

        LOG_INFO("Resource", "默认着色器资产已创建");
    } catch (const std::exception& e) {
        LOG_ERROR("Resource", "创建着色器资产失败: {0}", e.what());
    }
}

void ResourceManager::CreateDefaultTextures(const std::filesystem::path& texturesDir) {
    LOG_INFO("Resource", "创建默认纹理资产...");

    try {
        // 创建白色纹理
        TextureAsset whiteTexture;
        whiteTexture.SetMetadata("White", "纯白色纹理");
        whiteTexture.SetDimensions(1, 1, 4);
        std::vector<uint8_t> whiteData = {255, 255, 255, 255};
        whiteTexture.SetData(whiteData);
        whiteTexture.SerializeToFile(texturesDir / "White.texture", Serialization::SerializationFormat::JSON);

        // 创建黑色纹理
        TextureAsset blackTexture;
        blackTexture.SetMetadata("Black", "纯黑色纹理");
        blackTexture.SetDimensions(1, 1, 4);
        std::vector<uint8_t> blackData = {0, 0, 0, 255};
        blackTexture.SetData(blackData);
        blackTexture.SerializeToFile(texturesDir / "Black.texture", Serialization::SerializationFormat::JSON);

        // 创建红色纹理
        TextureAsset redTexture;
        redTexture.SetMetadata("Red", "纯红色纹理");
        redTexture.SetDimensions(1, 1, 4);
        std::vector<uint8_t> redData = {255, 0, 0, 255};
        redTexture.SetData(redData);
        redTexture.SerializeToFile(texturesDir / "Red.texture", Serialization::SerializationFormat::JSON);

        // 创建绿色纹理
        TextureAsset greenTexture;
        greenTexture.SetMetadata("Green", "纯绿色纹理");
        greenTexture.SetDimensions(1, 1, 4);
        std::vector<uint8_t> greenData = {0, 255, 0, 255};
        greenTexture.SetData(greenData);
        greenTexture.SerializeToFile(texturesDir / "Green.texture", Serialization::SerializationFormat::JSON);

        // 创建蓝色纹理
        TextureAsset blueTexture;
        blueTexture.SetMetadata("Blue", "纯蓝色纹理");
        blueTexture.SetDimensions(1, 1, 4);
        std::vector<uint8_t> blueData = {0, 0, 255, 255};
        blueTexture.SetData(blueData);
        blueTexture.SerializeToFile(texturesDir / "Blue.texture", Serialization::SerializationFormat::JSON);

        // 创建法线贴图（平法线）
        TextureAsset normalTexture;
        normalTexture.SetMetadata("FlatNormal", "平法线贴图");
        normalTexture.SetDimensions(1, 1, 4);
        std::vector<uint8_t> normalData = {128, 128, 255, 255};  // RGB = (0.5, 0.5, 1.0) 表示平法线
        normalTexture.SetData(normalData);
        normalTexture.SerializeToFile(texturesDir / "FlatNormal.texture", Serialization::SerializationFormat::JSON);

        // 创建金属度贴图（非金属）
        TextureAsset nonMetallicTexture;
        nonMetallicTexture.SetMetadata("NonMetallic", "非金属度贴图");
        nonMetallicTexture.SetDimensions(1, 1, 4);
        std::vector<uint8_t> nonMetallicData = {0, 0, 0, 255};  // R = 0 表示非金属
        nonMetallicTexture.SetData(nonMetallicData);
        nonMetallicTexture.SerializeToFile(texturesDir / "NonMetallic.texture",
                                           Serialization::SerializationFormat::JSON);

        // 创建粗糙度贴图（中等粗糙度）
        TextureAsset mediumRoughnessTexture;
        mediumRoughnessTexture.SetMetadata("MediumRoughness", "中等粗糙度贴图");
        mediumRoughnessTexture.SetDimensions(1, 1, 4);
        std::vector<uint8_t> mediumRoughnessData = {128, 128, 128, 255};  // R = 0.5 表示中等粗糙度
        mediumRoughnessTexture.SetData(mediumRoughnessData);
        mediumRoughnessTexture.SerializeToFile(texturesDir / "MediumRoughness.texture",
                                               Serialization::SerializationFormat::JSON);

        // 创建AO贴图（无遮挡）
        TextureAsset noAOTexture;
        noAOTexture.SetMetadata("NoAO", "无环境光遮挡贴图");
        noAOTexture.SetDimensions(1, 1, 4);
        std::vector<uint8_t> noAOData = {255, 255, 255, 255};  // R = 1 表示无遮挡
        noAOTexture.SetData(noAOData);
        noAOTexture.SerializeToFile(texturesDir / "NoAO.texture", Serialization::SerializationFormat::JSON);

        LOG_INFO("Resource", "默认纹理资产已创建");
    } catch (const std::exception& e) {
        LOG_ERROR("Resource", "创建纹理资产失败: {0}", e.what());
    }
}

void ResourceManager::CreateDefaultMaterials(const std::filesystem::path& materialsDir) {
    LOG_INFO("Resource", "创建默认材质资产...");

    try {
        // 创建基本材质
        std::ofstream basicMaterial(materialsDir / "Basic.material");
        basicMaterial << R"({
    "name": "Basic",
    "description": "基本材质，使用顶点颜色",
    "shader": {
        "vertex": "Shaders/BasicVS.hlsl",
        "pixel": "Shaders/BasicPS.hlsl"
    },
    "properties": {
        "diffuse": [1.0, 1.0, 1.0, 1.0]
    }
})";
        basicMaterial.close();

        // 创建纹理材质
        std::ofstream texturedMaterial(materialsDir / "Textured.material");
        texturedMaterial << R"({
    "name": "Textured",
    "description": "纹理材质，使用漫反射纹理",
    "shader": {
        "vertex": "Shaders/BasicVS.hlsl",
        "pixel": "Shaders/TexturedPS.hlsl"
    },
    "properties": {
        "diffuse": [1.0, 1.0, 1.0, 1.0]
    },
    "textures": {
        "diffuse": "Textures/White.texture"
    }
})";
        texturedMaterial.close();

        // 创建PBR材质
        std::ofstream pbrMaterial(materialsDir / "PBR.material");
        pbrMaterial << R"({
    "name": "PBR",
    "description": "物理基础渲染材质",
    "shader": {
        "vertex": "Shaders/PBRVS.hlsl",
        "pixel": "Shaders/PBRPS.hlsl"
    },
    "properties": {
        "albedo": [1.0, 1.0, 1.0, 1.0],
        "metallic": 0.0,
        "roughness": 0.5,
        "ao": 1.0
    },
    "textures": {
        "albedo": "Textures/White.texture",
        "normal": "Textures/FlatNormal.texture",
        "metallic": "Textures/NonMetallic.texture",
        "roughness": "Textures/MediumRoughness.texture",
        "ao": "Textures/NoAO.texture"
    }
})";
        pbrMaterial.close();

        // 创建金属材质
        std::ofstream metalMaterial(materialsDir / "Metal.material");
        metalMaterial << R"({
    "name": "Metal",
    "description": "金属材质",
    "shader": {
        "vertex": "Shaders/PBRVS.hlsl",
        "pixel": "Shaders/PBRPS.hlsl"
    },
    "properties": {
        "albedo": [0.7, 0.7, 0.8, 1.0],
        "metallic": 1.0,
        "roughness": 0.2,
        "ao": 1.0
    },
    "textures": {
        "albedo": "Textures/White.texture",
        "normal": "Textures/FlatNormal.texture",
        "metallic": "Textures/White.texture",
        "roughness": "Textures/Black.texture",
        "ao": "Textures/NoAO.texture"
    }
})";
        metalMaterial.close();

        // 创建塑料材质
        std::ofstream plasticMaterial(materialsDir / "Plastic.material");
        plasticMaterial << R"({
    "name": "Plastic",
    "description": "塑料材质",
    "shader": {
        "vertex": "Shaders/PBRVS.hlsl",
        "pixel": "Shaders/PBRPS.hlsl"
    },
    "properties": {
        "albedo": [0.8, 0.2, 0.2, 1.0],
        "metallic": 0.0,
        "roughness": 0.6,
        "ao": 1.0
    },
    "textures": {
        "albedo": "Textures/Red.texture",
        "normal": "Textures/FlatNormal.texture",
        "metallic": "Textures/NonMetallic.texture",
        "roughness": "Textures/MediumRoughness.texture",
        "ao": "Textures/NoAO.texture"
    }
})";
        plasticMaterial.close();

        LOG_INFO("Resource", "默认材质资产已创建");
    } catch (const std::exception& e) {
        LOG_ERROR("Resource", "创建材质资产失败: {0}", e.what());
    }
}

}  // namespace Engine