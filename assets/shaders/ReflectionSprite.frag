#version 450
layout(location = 0) in vec4 v_Color;
layout(location = 1) in vec2 v_UV;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D AlbedoMap;
layout(binding = 1) uniform sampler2D ReflectionSource;

// binding=2 reserved for NormalMap (future use)
// Reflection parameters via UBO (binding=3) avoids push constant
// conflicts with Renderer2D.vert.spv which uses offsets 0-79 for MVP+Color
layout(binding = 3) uniform ReflectionUBO {
    vec2  u_ReflectOffset;     // Reflection UV offset (e.g. (0, 1) = vertical flip)
    float u_ReflectStrength;   // Reflection intensity [0, 1]
    float u_FadeDistance;      // Distance-based fade multiplier
    float u_Distortion;        // Normal map distortion strength (unused in v1)
    vec4  u_TintColor;         // Reflection tint color
};

void main() {
    vec4 texColor = texture(AlbedoMap, v_UV);

    // Calculate reflection UV
    vec2 reflectUV = v_UV + u_ReflectOffset;

    // Sample reflection source at reflected coordinates
    vec4 reflected = texture(ReflectionSource, reflectUV);

    // Edge fade: reflections weaken near texture edges
    float fade = 1.0 - abs(reflectUV.y - 0.5) * 2.0;
    fade = smoothstep(0.0, 1.0, fade);

    // Distance-based fade
    float distFade = 1.0 - clamp(abs(reflectUV.y - 0.5) * u_FadeDistance, 0.0, 1.0);

    // Composite: Albedo + Reflected x Strength x Tint x Fade
    vec4 reflectionContrib = reflected * u_ReflectStrength
                           * vec4(u_TintColor.rgb, u_TintColor.a)
                           * fade * distFade;

    outColor = v_Color * texColor + reflectionContrib;
}
