#version 460 core
// ============================================================
// deferred_composite_copy.frag — Simple passthrough composite
//
// Samples a single HDR/LDR texture and outputs it directly.
// Used when compute-based tonemapping/SSR/fog have already run.
// Push Constants: exposure, gamma (for consistency)
// ============================================================

layout(location = 0) in vec2 v_TexCoord;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D InputBuffer;

layout(push_constant) uniform CompositionPushConstants {
    float exposure;
    float gamma;
    int debugView;
    float padding;
} pc;

void main() {
    vec3 color = texture(InputBuffer, v_TexCoord).rgb;
    outColor = vec4(color, 1.0);
}
