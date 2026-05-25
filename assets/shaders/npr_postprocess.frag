#version 450

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D sceneTexture;
layout(set = 0, binding = 1) uniform sampler2D bloomTexture;

layout(set = 1, binding = 0, std140) uniform PostProcessParams {
    float exposure;
    float bloomThreshold;
    float bloomIntensity;
    vec3 colorTemperature;
    float vignetteStrength;
} params;

// Inline NPR_ToneMap from npr_common.glsl
vec3 NPR_ToneMap(vec3 color, float exposure) {
    color = color * exposure;
    color = color / (color + vec3(1.0));
    return mix(color, color * vec3(1.0, 0.95, 0.85), 0.3);
}

void main() {
    vec3 color = texture(sceneTexture, inUV).rgb;

    // Bloom extraction
    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    vec3 bright = max(color - params.bloomThreshold, vec3(0.0));

    // Sample bloom texture
    vec3 bloom = texture(bloomTexture, inUV).rgb;

    // Composite bloom
    color += bloom * params.bloomIntensity;

    // NPR tone map
    color = NPR_ToneMap(color, params.exposure);

    // Color temperature (warm shift)
    color = mix(color, color * params.colorTemperature, 0.3);

    // Vignette
    vec2 center = inUV - 0.5;
    float vignette = 1.0 - dot(center, center) * params.vignetteStrength;
    color *= vignette;

    // Gamma correction
    color = pow(max(color, vec3(0.0)), vec3(1.0 / 2.2));

    outColor = vec4(color, 1.0);
}
