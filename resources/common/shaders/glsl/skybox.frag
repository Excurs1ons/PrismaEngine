#version 460 core
// ============================================================
// skybox.frag — PrismaEngine 天空盒片元着色器
//
// 支持两种模式:
//   1. 立方体贴图采样 (HDR 环境贴图)
//   2. 程序化天空 (基于方向 Y 分量的渐变)
// ============================================================

// ---- 片元输入 (来自 skybox.vert) ----
layout(location = 0) in vec3 v_LocalPos;

layout(location = 0) out vec4 outColor;

// ==================== Set 0: 天空盒纹理 ====================

layout(set = 0, binding = 0) uniform samplerCube SkyboxMap;

// ==================== Push Constants ====================

layout(push_constant) uniform SkyboxPushConstants {
    mat4 viewProjection;
} pc;

// ===================== 程序化天空 =====================

vec3 ProceduralSky(vec3 dir) {
    // 基于 Y 方向分量的渐变
    float y = dir.y;

    // 上部: 蓝色天空
    vec3 topColor = vec3(0.4, 0.6, 1.0);
    // 下部: 淡蓝/白色
    vec3 bottomColor = vec3(0.8, 0.85, 0.95);
    // 地平线: 暖色
    vec3 horizonColor = vec3(0.9, 0.7, 0.5);

    float horizonBlend = 1.0 - abs(y);
    vec3 color = mix(bottomColor, topColor, max(y, 0.0));

    // 地平线暖色混合
    color = mix(color, horizonColor, horizonBlend * 0.3);

    return color;
}

// ===================== 主函数 =====================

void main() {
    vec3 dir = normalize(v_LocalPos);

    // 采样立方体贴图 (HDR 环境贴图)
    vec3 skyColor = texture(SkyboxMap, dir).rgb;

    // 如果立方体贴图为黑色/无效, 使用程序化天空
    if (length(skyColor) < 0.001) {
        skyColor = ProceduralSky(dir);
    }

    // 简单色调映射 (天空盒通常是 HDR)
    skyColor = skyColor / (skyColor + vec3(1.0));

    outColor = vec4(skyColor, 1.0);
}
