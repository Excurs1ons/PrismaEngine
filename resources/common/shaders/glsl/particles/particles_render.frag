#version 460

// ============================================================================
// PrismaEngine — 粒子渲染片段着色器
// 圆形渐变纹理 + Alpha/Additive 混合支持
// ============================================================================

in vec2  v_TexCoord;
in vec4  v_Color;
flat in float v_Lifetime;
flat in float v_Age;

layout(location = 0) out vec4 fragColor;

void main() {
    // 基于到中心的距离创建圆形遮罩
    vec2 centered = v_TexCoord - vec2(0.5);
    float dist = length(centered);

    // 丢弃圆形外的像素
    if (dist > 0.5) discard;

    // 软边缘（平滑渐变）
    float alpha = 1.0 - smoothstep(0.3, 0.5, dist);

    // 根据年龄计算生命衰减
    float normalizedAge = v_Lifetime > 0.0 ? clamp(v_Age / v_Lifetime, 0.0, 1.0) : 1.0;
    float fadeAlpha = 1.0 - normalizedAge;

    // 最终颜色
    fragColor = vec4(v_Color.rgb, v_Color.a * alpha * fadeAlpha);

    // 注：Additive 和 Alpha 混合模式在管线状态中配置
    // - Alpha:    blend = (src * src_alpha) + (dst * (1 - src_alpha))
    // - Additive: blend = (src * src_alpha) + (dst * 1)
}
