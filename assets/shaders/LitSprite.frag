#version 450
layout(location = 0) in vec4 v_Color;
layout(location = 1) in vec2 v_UV;
layout(location = 0) out vec4 outColor;

// Descriptor 布局必须与 Material::UpdateDescriptorSetPBR() 完全一致
// (6 bindings: 0=UBO, 1-5=Sampler2D)，否则 VkDescriptorSetLayout 不兼容 → 黑屏。
// Binding 2 实际期望 LightMap（光照累加纹理），Layout 中名为 NormalMap（PBR 命名），
// Material 侧暂未绑定 LightMap，Fallback 为白色纹理（全亮，不影响渲染正确性）。
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
    // LightMap 读 binding 2（与 Material PBR 的 NormalMap slot 对齐，Material 侧 Fallback 为白色）
    vec4 lightColor = texture(LightMap, gl_FragCoord.xy / textureSize(LightMap, 0));
    outColor = v_Color * texColor * vec4(lightColor.rgb, 1.0);
}
