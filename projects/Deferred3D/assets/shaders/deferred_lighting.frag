#version 450
layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D gbPosition;
layout(binding = 1) uniform sampler2D gbNormal;
layout(binding = 2) uniform sampler2D gbAlbedo;
layout(binding = 3) uniform sampler2D gbEmissive;

layout(push_constant) uniform PushConstants {
    vec4 ambient;
    vec4 lightDir;
    vec4 lightColor;
    vec4 lightPos;
} pc;

void main() {
    vec3 N = normalize(texture(gbNormal, inUV).xyz);
    vec3 albedo = texture(gbAlbedo, inUV).rgb;
    vec3 emissive = texture(gbEmissive, inUV).rgb;

    vec3 ambient = pc.ambient.rgb * albedo;
    vec3 L = normalize(pc.lightDir.xyz);
    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = albedo * pc.lightColor.rgb * NdotL;

    vec3 color = ambient + diffuse + emissive;
    outColor = vec4(color, 1.0);
}
