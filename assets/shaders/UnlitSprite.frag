#version 450
layout(location = 0) in vec4 v_Color;
layout(location = 1) in vec2 v_UV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0, std140) uniform MaterialData {
    vec4 baseColor;
    float metallic;
    float roughness;
    float ao;
    float emissiveIntensity;
} material;

layout(set = 0, binding = 1) uniform sampler2D AlbedoMap;

void main() {
    vec4 texColor = texture(AlbedoMap, v_UV);
    float alpha = texColor.a * material.baseColor.a;
    // 输出预乘 alpha
    outColor = vec4(v_Color.rgb * texColor.rgb, v_Color.a * alpha);
}
