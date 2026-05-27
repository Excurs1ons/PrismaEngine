#version 450
// NPR (Non-Photorealistic) path tracing present shader
// Applies Reinhard tone mapping with warm color shift and vignette
layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D presentTexture;

void main() {
    vec4 color = texture(presentTexture, inUV);

    // NPR Tone Mapping (Reinhard + warm shift)
    vec3 mapped = color.rgb / (color.rgb + vec3(1.0));

    // Warm color shift (Sky style)
    mapped *= vec3(1.0, 0.95, 0.85);

    // Vignette
    vec2 center = inUV - 0.5;
    float vignette = 1.0 - dot(center, center) * 0.5;
    mapped *= vignette;

    // Gamma correction
    outColor = vec4(pow(max(mapped, vec3(0.0)), vec3(1.0 / 2.2)), 1.0);
}
