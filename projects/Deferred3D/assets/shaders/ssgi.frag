#version 450
// SSGI — 屏幕空间全局光照（单反弹）
// 对每个像素沿法线半球发射光线，步进检测表面，提取颜色

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outIndirect;

layout(binding = 0) uniform sampler2D gbPosition;
layout(binding = 1) uniform sampler2D gbNormal;
layout(binding = 2) uniform sampler2D gbAlbedo;
layout(binding = 3) uniform sampler2D gbEmissive;
layout(binding = 4) uniform sampler2D gbDepth;
layout(binding = 5) uniform sampler2D lightingResult;

layout(push_constant) uniform PushConstants {
    mat4 vp;               // view-projection
    vec4 cameraPos;
    vec4 params;           // x: 每像素光线数, y: 强度, z: 最大步进距离, w: 步长
} pc;

float hash(vec2 p) {
    vec2 r = p * 127.1 + vec2(269.5, 183.3);
    return fract(sin(dot(r, vec2(113.1, 73.7))) * 43758.5453);
}

// 构造法线半球方向（余弦权重）
vec3 sampleHemisphere(vec3 N, vec2 r) {
    float theta = 6.2831853 * r.x;
    float phi = acos(sqrt(1.0 - r.y));
    float x = sin(phi) * cos(theta);
    float y = cos(phi);
    float z = sin(phi) * sin(theta);
    vec3 T = normalize(cross(N, abs(N.y) < 0.99 ? vec3(0, 1, 0) : vec3(1, 0, 0)));
    vec3 B = cross(N, T);
    return x * T + y * N + z * B;
}

// 从深度纹理还原视空间深度
float linearizeDepth(float d, float near, float far) {
    return near * far / (far + d * (near - far));
}

void main() {
    vec3 worldPos = texture(gbPosition, inUV).xyz;
    vec3 N = normalize(texture(gbNormal, inUV).xyz);
    float centerDepth = texture(gbDepth, inUV).r;

    if (centerDepth > 0.999) {
        outIndirect = vec4(0, 0, 0, 1);
        return;
    }

    int rayCount = int(pc.params.x + 0.5);
    float intensity = pc.params.y;
    float maxDist = pc.params.z;
    float stepSize = pc.params.w;

    vec3 indirect = vec3(0);
    float totalWeight = 0.0;

    for (int i = 0; i < rayCount; i++) {
        vec2 r;
        r.x = hash(inUV + vec2(float(i) * 0.311, 0.0));
        r.y = hash(inUV + vec2(0.0, float(i) * 0.457));

        vec3 dir = sampleHemisphere(N, r);

        // 沿光线方向步进
        for (float t = stepSize; t < maxDist; t += stepSize) {
            vec3 pos = worldPos + dir * t;

            // 投影到屏幕空间
            vec4 proj = pc.vp * vec4(pos, 1.0);
            vec2 uv = proj.xy / proj.w * 0.5 + 0.5;

            if (any(lessThan(uv, vec2(0.0))) || any(greaterThan(uv, vec2(1.0))))
                break; // 出屏，放弃此光线

            float sampleDepth = texture(gbDepth, uv).r;
            if (sampleDepth > 0.999) continue; // 背景，继续步进

            vec3 hitPos = texture(gbPosition, uv).xyz;
            float hitDist = length(hitPos - worldPos);
            float rayDist = length(pos - worldPos);

            // 击中检测：光线位置和 GBuffer 表面位置足够近
            float dist3D = length(pos - hitPos);
            if (dist3D < stepSize * 1.5) {
                // 命中！读取该点的已照亮颜色
                vec3 hitColor = texture(lightingResult, uv).rgb;
                vec3 hitN = normalize(texture(gbNormal, uv).xyz);

                // 几何权重：接收面法线·光线方向 × 发射面法线·-光线方向
                vec3 toHit = normalize(hitPos - worldPos);
                float geom = max(dot(N, toHit), 0.0) * max(dot(hitN, -toHit), 0.0);
                if (geom < 0.01) break;

                float atten = 1.0 / (1.0 + hitDist * hitDist * 0.3);

                indirect += hitColor * geom * atten;
                totalWeight += geom * atten;
                break; // 此光线已完成
            }

            // 如果光线穿过表面后面（光线深度超出表面），放弃
            float rayViewZ = linearizeDepth(proj.z / proj.w * 0.5 + 0.5, 0.1, 100.0);
            float hitViewZ = linearizeDepth(sampleDepth, 0.1, 100.0);
            if (rayViewZ > hitViewZ + stepSize * 2.0) {
                break; // 光线跑到表面后面去了
            }
        }
    }

    if (totalWeight > 0.001 && rayCount > 0) {
        indirect = indirect * intensity / float(rayCount);
    }

    outIndirect = vec4(indirect, 1.0);
}
