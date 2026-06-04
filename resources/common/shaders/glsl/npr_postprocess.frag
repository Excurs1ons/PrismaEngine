#version 460 core
// ============================================================
// npr_postprocess.frag — PrismaEngine NPR 后处理片元着色器
//
// 应用于 NPR 渲染结果的后处理效果:
//   - Sobel 边缘检测 (绘制物体轮廓线)
//   - 颜色量化 (后处理阶段)
//   - 可选: 在边缘处叠加黑色线条
// ============================================================

// ---- 片元输入 ----
layout(location = 0) in vec2 v_TexCoord;

layout(location = 0) out vec4 outColor;

// ==================== Set 0: 输入 ====================

layout(set = 0, binding = 0) uniform sampler2D SceneColor;   // NPR 渲染结果
layout(set = 0, binding = 1) uniform sampler2D SceneDepth;   // 深度缓冲

// ==================== Push Constants ====================

layout(push_constant) uniform NPRPostProcessPushConstants {
    float edgeThreshold;    // 边缘检测阈值 (默认: 0.1)
    float quantizeStrength; // 颜色量化强度 (0~1)
    float outlineWidth;     // 轮廓线宽度 (像素)
    float edgeColorIntensity; // 边缘颜色强度
} pc;

// ===================== Sobel 边缘检测 =====================

float Luminance(vec3 color) {
    return dot(color, vec3(0.299, 0.587, 0.114));
}

float SobelEdgeDetection(sampler2D depthTex, vec2 uv, vec2 texelSize) {
    // 深度梯度 Sobel 算子
    float tl = texture(depthTex, uv + vec2(-1.0, -1.0) * texelSize).r;
    float t  = texture(depthTex, uv + vec2( 0.0, -1.0) * texelSize).r;
    float tr = texture(depthTex, uv + vec2( 1.0, -1.0) * texelSize).r;
    float l  = texture(depthTex, uv + vec2(-1.0,  0.0) * texelSize).r;
    float r  = texture(depthTex, uv + vec2( 1.0,  0.0) * texelSize).r;
    float bl = texture(depthTex, uv + vec2(-1.0,  1.0) * texelSize).r;
    float b  = texture(depthTex, uv + vec2( 0.0,  1.0) * texelSize).r;
    float br = texture(depthTex, uv + vec2( 1.0,  1.0) * texelSize).r;

    // Sobel X
    float sobelX = tr + 2.0 * r + br - tl - 2.0 * l - bl;
    // Sobel Y
    float sobelY = bl + 2.0 * b + br - tl - 2.0 * t - tr;

    float edge = sqrt(sobelX * sobelX + sobelY * sobelY);
    return edge;
}

float SobelEdgeDetectionColor(sampler2D colorTex, vec2 uv, vec2 texelSize) {
    // 基于亮度的 Sobel
    float tl = Luminance(texture(colorTex, uv + vec2(-1.0, -1.0) * texelSize).rgb);
    float t  = Luminance(texture(colorTex, uv + vec2( 0.0, -1.0) * texelSize).rgb);
    float tr = Luminance(texture(colorTex, uv + vec2( 1.0, -1.0) * texelSize).rgb);
    float l  = Luminance(texture(colorTex, uv + vec2(-1.0,  0.0) * texelSize).rgb);
    float r  = Luminance(texture(colorTex, uv + vec2( 1.0,  0.0) * texelSize).rgb);
    float bl = Luminance(texture(colorTex, uv + vec2(-1.0,  1.0) * texelSize).rgb);
    float b  = Luminance(texture(colorTex, uv + vec2( 0.0,  1.0) * texelSize).rgb);
    float br = Luminance(texture(colorTex, uv + vec2( 1.0,  1.0) * texelSize).rgb);

    float sobelX = tr + 2.0 * r + br - tl - 2.0 * l - bl;
    float sobelY = bl + 2.0 * b + br - tl - 2.0 * t - tr;

    return sqrt(sobelX * sobelX + sobelY * sobelY);
}

// ===================== 颜色量化 =====================

vec3 QuantizeColor(vec3 color, float strength, float levels) {
    vec3 quantized = floor(color * levels) / levels;
    return mix(color, quantized, strength);
}

// ===================== 主函数 =====================

void main() {
    vec2 texelSize = 1.0 / textureSize(SceneColor, 0);

    // 采样场景颜色
    vec3 sceneColor = texture(SceneColor, v_TexCoord).rgb;

    // 检测边缘 (基于深度和颜色)
    float depthEdge = SobelEdgeDetection(SceneDepth, v_TexCoord, texelSize);
    float colorEdge = SobelEdgeDetectionColor(SceneColor, v_TexCoord, texelSize);

    // 合并边缘
    float edge = max(depthEdge, colorEdge);

    // 阈值判定
    float outline = step(pc.edgeThreshold, edge);

    // 颜色量化
    vec3 finalColor = QuantizeColor(sceneColor, pc.quantizeStrength, 8.0);

    // 叠加黑色轮廓线
    vec3 outlineColor = mix(finalColor, vec3(0.0), outline * pc.edgeColorIntensity);

    outColor = vec4(outlineColor, 1.0);
}
