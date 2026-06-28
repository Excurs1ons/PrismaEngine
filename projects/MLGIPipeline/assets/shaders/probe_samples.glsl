#ifndef PROBE_SAMPLES_GLSL
#define PROBE_SAMPLES_GLSL

// ============================================================================
// probe_samples.glsl — deterministic sample source for probe ray directions
//
// This header is #include'd by probe update shaders. The CPU-side
// SampleSource class generates matching data and uploads it as an SSBO.
//
// Two modes are supported (see SampleMode enum in SampleSource.h):
//   mode == 0: Halton(2) for x, Halton(3) for y
//   mode == 1: Hammersley 2D (i/N, radicalInverse(i,2))
//
// SSBO layout (std430):
//   layout(std430, binding = N) readonly buffer ProbeSamples {
//       uint sampleCount;   // [0] total number of samples
//       uint seed;          // [1] seed used for generation
//       uint mode;          // [2] 0=Halton, 1=Hammersley
//       uint _pad;          // [3] padding (16-byte header)
//       vec2 samples[];     // [4..] sample positions in [0,1]^2
//   } probeSamples;
// ============================================================================

// ── Sample retrieval ──
// Get a 2D probe sample by index (wraps via modulo)
vec2 getProbeSample(uint index) {
    uint idx = index % probeSamples.sampleCount;
    return probeSamples.samples[idx];
}

// ── Hemisphere direction from 2D sample ──
// Convert a 2D uniform sample to a cosine-weighted hemisphere direction
// using the given normal as the hemisphere axis.
vec3 sampleHemisphere(vec2 u, vec3 normal) {
    float r = sqrt(u.x);
    float theta = 6.283185307 * u.y;

    vec3 tangent = abs(normal.y) < 0.9999
        ? vec3(0.0, 1.0, 0.0)
        : vec3(1.0, 0.0, 0.0);
    vec3 bitangent = normalize(cross(normal, tangent));
    tangent = cross(bitangent, normal);

    return tangent * r * cos(theta)
         + bitangent * r * sin(theta)
         + normal * sqrt(max(0.0, 1.0 - u.x));
}

// ── Convenience: get a probe ray direction directly ──
vec3 getProbeRayDir(uint sampleIndex, vec3 normal) {
    vec2 u = getProbeSample(sampleIndex);
    return sampleHemisphere(u, normal);
}

#endif // PROBE_SAMPLES_GLSL
