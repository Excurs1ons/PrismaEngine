#version 460

// ============================================================================
// PrismaEngine — Water Fragment Shader
// Fresnel (Schlick), reflection + refraction, depth fog, specular
// ============================================================================

layout(location = 0) in vec3 v_WorldPos;
layout(location = 1) in vec3 v_Normal;
layout(location = 2) in vec2 v_TexCoord;
layout(location = 3) in vec3 v_Tangent;
layout(location = 4) in vec3 v_Bitangent;
layout(location = 5) in vec3 v_ViewDir;
layout(location = 6) in float v_WaveHeight;
layout(location = 7) flat in float v_FarPlane;

layout(location = 0) out vec4 fragColor;

// Camera uniforms
layout(std140, binding = 0) uniform CameraUniforms {
    mat4  u_ViewProjection;
    mat4  u_View;
    mat4  u_Projection;
    vec3  u_CameraPos;
    float u_Near;
    vec3  u_CameraForward;
    float u_Far;
} camera;

// Water uniforms
layout(std140, binding = 1) uniform WaterUniforms {
    vec4  u_DeepColor;
    vec4  u_ShallowColor;
    vec4  u_FogColor;
    float u_WaterLevel;
    float u_WaveHeightScale;
    float u_FresnelPower;
    float u_FresnelStrength;
    float u_SpecularStrength;
    float u_SpecularPower;
    float u_Transparency;
    float u_RefractionScale;
    float u_SunGlowStrength;
    float u_SunGlowPower;
    float u_Time;
    float u_FogDensity;
    float u_ShallowDepth;
    float u_DeepDepth;
} water;

// Reflection cubemap or reflection texture
layout(binding = 3) uniform samplerCube u_ReflectionMap;
// Refraction scene texture
layout(binding = 4) uniform sampler2D u_RefractionMap;
// Depth texture for depth-based effects
layout(binding = 5) uniform sampler2D u_DepthMap;

// Sun direction (for specular / sun glow)
const vec3 sunDir = normalize(vec3(0.5, 0.8, 0.3));

// Screen space UV
vec2 getScreenUV(vec4 clipPos) {
    return vec2(clipPos.xy / clipPos.w) * 0.5 + 0.5;
}

// Schlick Fresnel approximation
float fresnelSchlick(float cosTheta, float f0) {
    return f0 + (1.0 - f0) * pow(1.0 - cosTheta, 5.0);
}

// Normal mapping from wave perturbation
vec3 getPerturbedNormal(vec3 normal, vec3 tangent, vec3 bitangent) {
    // Use wave height gradient as normal perturbation
    // In a full implementation, this would sample a normal map
    return normalize(normal);
}

// Linearize depth from non-linear depth buffer
float linearizeDepth(float depth, float near, float far) {
    float z = depth * 2.0 - 1.0;
    return (2.0 * near * far) / (far + near - z * (far - near));
}

void main() {
    vec3 N = normalize(v_Normal);
    vec3 V = normalize(v_ViewDir);

    // Fresnel effect (Schlick approximation)
    float cosTheta = max(dot(N, V), 0.0);
    float f0 = 0.02 + water.u_FresnelStrength * 0.04; // Base reflectivity
    float fresnel = fresnelSchlick(cosTheta, f0);

    // Apply Fresnel power for artistic control
    fresnel = pow(fresnel, water.u_FresnelPower);

    // ---- Reflection ----
    vec3 reflectDir = reflect(-V, N);
    vec4 reflectionColor = texture(u_ReflectionMap, reflectDir);

    // ---- Refraction ----
    vec4 clipPos = camera.u_ViewProjection * vec4(v_WorldPos, 1.0);
    vec2 screenUV = getScreenUV(clipPos);

    // Refraction distortion: perturb UV by wave normal
    vec2 refractionOffset = N.xz * water.u_RefractionScale * v_WaveHeight;
    vec2 refractedUV = screenUV + refractionOffset;

    vec4 refractionColor = texture(u_RefractionMap, refractedUV);

    // Depth-based color blending (shallow vs deep)
    float sceneDepth = texture(u_DepthMap, screenUV).r;
    float linearSceneDepth = linearizeDepth(sceneDepth, camera.u_Near, camera.u_Far);
    float waterDepth = linearSceneDepth - v_WorldPos.y;
    float depthFactor = clamp((waterDepth - water.u_ShallowDepth) /
                              (water.u_DeepDepth - water.u_ShallowDepth), 0.0, 1.0);
    vec3 baseColor = mix(water.u_ShallowColor.rgb, water.u_DeepColor.rgb, depthFactor);

    // Blend refraction with base color based on transparency
    vec3 refracted = mix(refractionColor.rgb, baseColor, water.u_Transparency);

    // ---- Specular ----
    vec3 halfVec = normalize(V + sunDir);
    float specAngle = max(dot(N, halfVec), 0.0);
    float specular = water.u_SpecularStrength * pow(specAngle, water.u_SpecularPower);

    // ---- Sun Glow (波光粼粼) ----
    vec3 reflectSun = reflect(-sunDir, N);
    float sunGlowAngle = max(dot(V, reflectSun), 0.0);
    float sunGlow = water.u_SunGlowStrength * pow(sunGlowAngle, water.u_SunGlowPower);

    // ---- Composite ----
    // Fresnel blend: reflection at grazing angles, refraction/color at normal
    vec3 color = mix(refracted, reflectionColor.rgb, fresnel * water.u_FresnelStrength);

    // Add specular and sun glow
    color += specular * vec3(1.0, 0.95, 0.85); // Warm specular
    color += sunGlow * vec3(1.0, 0.9, 0.7);     // Warm sun glow

    // ---- Depth Fog ----
    float fogFactor = 1.0 - exp(-water.u_FogDensity * waterDepth);
    color = mix(color, water.u_FogColor.rgb, fogFactor);

    // ---- Alpha ----
    // Water alpha based on viewing angle and transparency
    float alpha = mix(water.u_Transparency, 1.0, fresnel);

    fragColor = vec4(color, alpha);
}
