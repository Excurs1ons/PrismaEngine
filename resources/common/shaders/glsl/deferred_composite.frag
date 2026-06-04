#version 460 core
// ============================================================
// deferred_composite.frag — PrismaEngine 延迟渲染合成片元着色器
//
// 最终合成 Pass:
//   - 合并光照结果与自发光
//   - Debug 视图 (法线, 深度, 粗糙度, 金属度等)
//   - 色调映射 + Gamma 校正
// ============================================================

// ---- 片元输入 (来自 deferred_fullscreen.vert) ----
layout(location = 0) in vec2 v_TexCoord;

layout(location = 0) out vec4 outColor;

// ==================== Set 0: 输入渲染目标 ====================

layout(set = 0, binding = 0) uniform sampler2D LightingBuffer;  // 光照计算结果
layout(set = 0, binding = 1) uniform sampler2D GBufferAlbedo;   // G-buffer 反照率
layout(set = 0, binding = 2) uniform sampler2D GBufferNormal;   // G-buffer 法线
layout(set = 0, binding = 3) uniform sampler2D GBufferEmissive; // G-buffer 自发光
layout(set = 0, binding = 4) uniform sampler2D GBufferDepth;    // G-buffer 深度

// ==================== Push Constants ====================

layout(push_constant) uniform CompositionPushConstants {
    float exposure;
    float gamma;
    int debugView;      // 0=final, 1=albedo, 2=normal, 3=depth, 4=roughness, 5=emissive
    float padding;
} pc;

// ===================== 调试可视化 =====================

vec3 DebugDepth(float depth) {
    // 将深度映射到颜色渐变
    float d = depth;
    return mix(vec3(0.02, 0.02, 0.3), vec3(1.0, 0.95, 0.8), d);
}

vec3 DebugNormal(vec3 normal) {
    return normal * 0.5 + 0.5;
}

vec3 DebugAlbedo(vec3 albedo) {
    return albedo;
}

vec3 DebugEmissive(vec3 emissive) {
    return emissive;
}

vec3 DebugRoughness(float roughness) {
    return vec3(roughness);
}

vec3 DebugMetallic(float metallic) {
    return vec3(metallic);
}

// ===================== 色调映射 =====================

vec3 ACESToneMapping(vec3 color, float exposure) {
    color *= exposure;
    const float A = 2.51;
    const float B = 0.03;
    const float C = 2.43;
    const float D = 0.59;
    const float E = 0.14;
    return (color * (A * color + B)) / (color * (C * color + D) + E);
}

vec3 GammaCorrection(vec3 color, float gamma) {
    return pow(max(color, vec3(0.0)), vec3(1.0 / gamma));
}

// ===================== 主函数 =====================

void main() {
    // 采样 G-buffer
    vec4 albedoAO     = texture(GBufferAlbedo, v_TexCoord);
    vec4 normalMetal  = texture(GBufferNormal, v_TexCoord);
    vec3 emissive     = texture(GBufferEmissive, v_TexCoord).rgb;
    float depth       = texture(GBufferDepth, v_TexCoord).r;

    vec3 albedo   = albedoAO.rgb;
    float ao      = albedoAO.a;
    vec3 N        = normalize(normalMetal.xyz * 2.0 - 1.0);
    float metallic = normalMetal.a;

    // ===== Debug View =====
    vec3 debugResult;
    switch (pc.debugView) {
        case 1: // 反照率
            debugResult = DebugAlbedo(albedo);
            break;
        case 2: // 法线
            debugResult = DebugNormal(N);
            break;
        case 3: // 深度
            debugResult = DebugDepth(depth);
            break;
        case 4: // 粗糙度
            debugResult = DebugRoughness(1.0 - albedoAO.a); // 使用 ao 近似
            break;
        case 5: // 自发光
            debugResult = DebugEmissive(emissive);
            break;
        default: // 最终合成 (0)
            {
                // 采样光照结果
                vec3 lighting = texture(LightingBuffer, v_TexCoord).rgb;

                // 合并光照 + 自发光
                vec3 finalColor = lighting + emissive;

                // 后处理
                finalColor = ACESToneMapping(finalColor, pc.exposure);
                finalColor = GammaCorrection(finalColor, pc.gamma);
                debugResult = finalColor;
            }
            break;
    }

    outColor = vec4(debugResult, 1.0);
}
