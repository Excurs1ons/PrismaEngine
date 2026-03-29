#version 460

// 纹理采样器（从精灵着色器的输出）
layout(binding = 0) uniform sampler2D u_AlbedoMap;      // 反照率（颜色）
layout(binding = 1) uniform sampler2D u_PositionMap;    // 世界坐标

// ========== 2D 光源 Uniform 缓冲区 ==========

// 2D 光源类型
const int LIGHT_TYPE_POINT = 0;
const int LIGHT_TYPE_DIRECTIONAL = 1;

struct Light2D {
    int type;                   // 光源类型
    vec2 position;              // 位置（世界坐标）
    vec3 color;                 // RGB 颜色
    float intensity;            // 强度
    float radius;               // 影响半径
    float falloff;              // 衰减系数
    bool castShadows;           // 是否投射阴影
};

layout(binding = 2) uniform LightUBO {
    vec3 u_AmbientLight;         // 环境光颜色
    int u_LightCount;             // 光源数量
    Light2D u_Lights[16];       // 最多16个光源
};

// ========== 输入 ==========

layout(location = 0) in vec2 v_TexCoord;     // 纹理坐标

// ========== 输出 ==========

layout(location = 0) out vec4 out_Color;       // 最终颜色

// ========== 辅助函数 ==========

/**
 * @brief 计算点光源的衰减
 */
float CalculateAttenuation(float distance, float radius, float falloff) {
    if (distance >= radius) {
        return 0.0f;
    }

    // 使用平滑衰减
    float normalizedDistance = distance / radius;
    float attenuation = 1.0f / (1.0f + falloff * distance * distance);

    // 边缘平滑衰减
    float edgeSmoothing = 1.0f - normalizedDistance;
    edgeSmoothing = clamp(edgeSmoothing, 0.0f, 1.0f);
    edgeSmoothing = edgeSmoothing * edgeSmoothing;

    return attenuation * edgeSmoothing;
}

/**
 * @brief 检查像素是否在阴影中（简化版）
 */
bool IsInShadow(vec2 pixelPos, vec2 lightPos) {
    // TODO: 实现真正的阴影映射
    // 这是一个简化的版本，检查像素附近是否有遮挡物
    return false;
}

// ========== 主函数 ==========

void main() {
    // 采样反照率
    vec4 albedo = texture(u_AlbedoMap, v_TexCoord);

    // 采样世界坐标
    vec4 worldPos = texture(u_PositionMap, v_TexCoord);
    vec2 pixelPosition = worldPos.xy;

    // 从环境光开始
    vec3 finalColor = u_AmbientLight;

    // 遍历所有光源
    for (int i = 0; i < u_LightCount; i++) {
        Light2D light = u_Lights[i];

        if (light.intensity <= 0.01f) {
            continue;
        }

        vec3 lightContribution = vec3(0.0f);

        // 处理点光源
        if (light.type == LIGHT_TYPE_POINT) {
            // 计算光源到像素的距离
            float distance = length(light.position - pixelPosition);

            // 计算衰减
            float attenuation = CalculateAttenuation(distance, light.radius, light.falloff);

            // 光源贡献
            lightContribution = light.color * light.intensity * attenuation;

            // 简单的阴影检查（性能优化：仅对靠近像素的光源进行）
            if (light.castShadows && distance < 32.0f && distance > 8.0f) {
                if (IsInShadow(pixelPosition, light.position)) {
                    lightContribution *= 0.3f;  // 阴影中减少光照
                }
            }
        }
        // 处理方向光（环境光）
        else if (light.type == LIGHT_TYPE_DIRECTIONAL) {
            lightContribution = light.color * light.intensity;
        }

        // 累加光照贡献
        finalColor += lightContribution;
    }

    // 应用光照到反照率
    vec3 resultColor = albedo.rgb * finalColor;

    // 输出最终颜色
    out_Color = vec4(resultColor, albedo.a.a);
}
