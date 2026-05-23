#version 450
layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D gbPosition;
layout(binding = 1) uniform sampler2D gbNormal;
layout(binding = 2) uniform sampler2D gbAlbedo;
layout(binding = 3) uniform sampler2D gbEmissive;

layout(binding = 4) uniform LightingData {
    vec4 ambient;
    vec4 lightDir;
    vec4 lightColor;
} ub;

float hash(vec2 p) {
    vec2 r = p * 127.1 + vec2(269.5, 183.3);
    return fract(sin(dot(r, vec2(113.1, 73.7))) * 43758.5453);
}

void main() {
    vec3 N = normalize(texture(gbNormal, inUV).xyz);
    vec3 albedo = texture(gbAlbedo, inUV).rgb;
    vec3 emissiveSelf = texture(gbEmissive, inUV).rgb;

    // 环境光（UBO 传递）
    vec3 ambient = ub.ambient.rgb * albedo;

    // 方向光
    vec3 diffuse = vec3(0);
    if (length(ub.lightColor.rgb) > 0.001 && length(ub.lightDir.xyz) > 0.001) {
        vec3 L = normalize(ub.lightDir.xyz);
        float NdotL = max(dot(N, L), 0.0);
        diffuse = albedo * ub.lightColor.rgb * NdotL;
    }

    // 硬编码 emissive 面光源直接照明（Cornell Box 天花板 Light 面）
    vec3 lightCenter = vec3(0.0, 0.995, 0.0);
    float halfSize = 0.3;
    vec3 le = vec3(10.0, 10.0, 10.0);
    vec3 emitterDirect = vec3(0);
    const int ns = 8;
    for (int i = 0; i < ns; i++) {
        vec2 r;
        r.x = hash(inUV + vec2(float(i) * 0.137, 0.0));
        r.y = hash(inUV + vec2(0.0, float(i) * 0.251));
        vec3 off = vec3((r.x - 0.5) * halfSize * 2.0, 0.0, (r.y - 0.5) * halfSize * 2.0);
        vec3 lp = lightCenter + off;
        vec3 wp = texture(gbPosition, inUV).xyz;
        if (length(wp) < 0.001) continue;
        vec3 toLight = lp - wp;
        float dist = length(toLight);
        if (dist < 0.001) continue;
        toLight /= dist;
        float ndotl = max(dot(N, toLight), 0.0);
        if (ndotl < 0.001) continue;
        float atten = 1.0 / (1.0 + dist * dist * 0.15);
        float backNdotL = max(dot(-toLight, vec3(0, -1, 0)), 0.0);
        emitterDirect += le * ndotl * backNdotL * atten;
    }
    emitterDirect = albedo * emitterDirect / float(ns) * 0.15;

    vec3 color = ambient + diffuse + emissiveSelf + emitterDirect;
    outColor = vec4(color, 1.0);
}
