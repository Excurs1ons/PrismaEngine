#version 450
layout(location = 0) in vec4 v_Color;
layout(location = 1) in vec2 v_UV;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform MaterialData {
    vec4 baseColor;
    float metallic;
    float roughness;
    float ao;
    float emissiveIntensity;
} matData;

layout(binding = 1) uniform sampler2D AlbedoMap;
layout(binding = 2) uniform sampler2D LightMap;          // Layout slot = NormalMap in PBR
layout(binding = 3) uniform sampler2D MetallicRoughnessMap;
layout(binding = 4) uniform sampler2D AOMap;
layout(binding = 5) uniform sampler2D EmissiveMap;

void main() {
    vec4 texColor = texture(AlbedoMap, v_UV);
    // LightMap 璇?binding 2锛堜笌 Material PBR 鐨?NormalMap slot 瀵归綈锛孧aterial 渚?Fallback 涓虹櫧鑹诧級
    vec4 lightColor = texture(LightMap, gl_FragCoord.xy / textureSize(LightMap, 0));
    outColor = v_Color * texColor * vec4(lightColor.rgb, 1.0);
}
