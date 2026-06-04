// ============================================================
// shadow.vert — PrismaEngine 阴影贴图顶点着色器
//
// 将顶点从模型空间变换到光照空间，并应用深度偏移。
// 用于级联阴影贴图 (CSM) 的渲染。
// Push Constants: 光照视投影矩阵, 深度偏移参数
// ============================================================

#version 460 core

// ---- 顶点输入 ----
layout(location = 0) in vec3 aPos;

// ---- Push Constants (匹配 C++ ShadowPushConstants) ----
layout(push_constant) uniform ShadowConstants {
    mat4 lightVP;           // 光照空间视投影矩阵
    float shadowBias;       // 深度偏移量
    float shadowNormalBias; // 法线方向偏移量
    uint cascadeIndex;      // 当前级联索引
    float cascadeCount;     // 总级联数
} pc;

void main() {
    // 标准模型-光照空间变换
    gl_Position = pc.lightVP * vec4(aPos, 1.0);

    // 应用深度偏移以减少阴影痤疮 (shadow acne)
    // 偏移量与深度成正比，避免远距离过度偏移
    const float depthSlopeScale = 1.0;
    float bias = pc.shadowBias * depthSlopeScale;
    gl_Position.z -= bias * gl_Position.w;
}
