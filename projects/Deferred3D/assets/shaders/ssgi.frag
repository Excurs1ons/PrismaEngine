#version 450
// SSGI (Screen Space Global Illumination) — 单反弹间接光照
// 使用 GBuffer + 深度在屏幕空间采样周围像素的颜色贡献

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outIndirect;

layout(binding = 0) uniform sampler2D gbPosition;
layout(binding = 1) uniform sampler2D gbNormal;
layout(binding = 2) uniform sampler2D gbAlbedo;
layout(binding = 3) uniform sampler2D gbEmissive;
layout(binding = 4) uniform sampler2D gbDepth;

layout(push_constant) uniform PushConstants {
    mat4 vp;                // view-projection matrix (for projecting sample positions)
    vec4 cameraPos;         // camera world position
    vec4 params;            // x: sampleRadius, y: sampleCount, z: intensity, w: padding
} pc;

// 简单哈希函数 (PCG-style)
float hash(vec2 p) {
    vec2 r = p * 127.1 + vec2(269.5, 183.3);
    return fract(sin(dot(r, vec2(113.1, 73.7))) * 43758.5453);
}

// 根据法线生成半球方向（余弦加权重要性采样）
vec3 sampleHemisphere(vec3 N, vec2 rand) {
    float theta = 2.0 * 3.14159265 * rand.x;
    float phi = acos(sqrt(1.0 - rand.y));  // 偏向法线方向
    float x = sin(phi) * cos(theta);
    float y = cos(phi);
    float z = sin(phi) * sin(theta);

    // 构造切空间基
    vec3 T = normalize(cross(N, abs(N.y) < 0.99 ? vec3(0, 1, 0) : vec3(1, 0, 0)));
    vec3 B = cross(N, T);
    return x * T + y * N + z * B;
}

// 从深度重建线性深度（NDC z [0,1] → 视空间距离）
float linearizeDepth(float d, float near, float far) {
    return near * far / (far + d * (near - far));
}

void main() {
    vec2 pixelSize = 1.0 / textureSize(gbDepth, 0);

    vec3 worldPos = texture(gbPosition, inUV).xyz;
    vec3 N = normalize(texture(gbNormal, inUV).xyz);
    float centerDepth = texture(gbDepth, inUV).r;

    vec3 indirect = vec3(0);
    float totalWeight = 0.0;

    // 深度不连续则跳过（天空/背景）
    if (centerDepth > 0.999) {
        outIndirect = vec4(0, 0, 0, 1);
        return;
    }

    int sampleCount = int(pc.params.y + 0.5);
    float radius = pc.params.x;
    float intensity = pc.params.z;

    for (int i = 0; i < sampleCount; i++) {
        // 每个像素/样本使用不同的随机种子
        vec2 rand;
        rand.x = hash(inUV + vec2(float(i) * 0.127, 0.0));
        rand.y = hash(inUV + vec2(0.0, float(i) * 0.271));

        // 生成半球方向
        vec3 dir = sampleHemisphere(N, rand);
        float dist = radius * (0.3 + rand.y * 0.7);  // 随机采样距离

        vec3 samplePos = worldPos + dir * dist;

        // 投影到屏幕空间
        vec4 proj = pc.vp * vec4(samplePos, 1.0);
        vec2 sampleUV = proj.xy / proj.w * 0.5 + 0.5;

        // 边界检查
        if (sampleUV.x < 0 || sampleUV.x > 1 ||
            sampleUV.y < 0 || sampleUV.y > 1) continue;

        // 读取采样点的深度和颜色
        float sampleDepth = texture(gbDepth, sampleUV).r;
        if (sampleDepth > 0.999) continue;  // 跳过背景

        vec3 sampleWorldPos = texture(gbPosition, sampleUV).xyz;
        vec3 sampleNormal = normalize(texture(gbNormal, sampleUV).xyz);

        // 深度连续性检查：只接受相近深度的采样
        float depthDiff = abs(samplePos.z - sampleWorldPos.z);
        if (depthDiff > radius * 0.5) continue;

        // 法线兼容性：只接受法线朝向采样点的表面
        vec3 toSample = normalize(sampleWorldPos - worldPos);
        float NdotV = max(dot(N, toSample), 0.0);
        float backNdotV = max(dot(sampleNormal, -toSample), 0.0);
        float visibility = NdotV * backNdotV;

        if (visibility < 0.01) continue;

        // 读取采样点的颜色（emissive + albedo 作为间接光贡献）
        vec3 albedo = texture(gbAlbedo, sampleUV).rgb;
        vec3 emissive = texture(gbEmissive, sampleUV).rgb;
        vec3 sampleColor = emissive + albedo;    // 间接光来自漫反射 + 自发光

        // 距离衰减
        float attenuation = 1.0 / (1.0 + dist * dist * 0.5);

        indirect += sampleColor * visibility * attenuation;
        totalWeight += visibility * attenuation;
    }

    if (totalWeight > 0.001) {
        indirect = indirect * intensity / totalWeight;
    }

    outIndirect = vec4(indirect, 1.0);
}
