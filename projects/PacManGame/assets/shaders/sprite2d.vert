#version 460

// ========== 输入顶点属性 ==========

layout(location = 0) in vec2 a_Position;      // 顶点位置
layout(location = 1) in vec2 a_TexCoord;     // 纹理坐标
layout(location = 2) in vec4 a_Color;         // 顶点颜色

// ========== Uniform 缓冲区 ==========

// 相机 Uniform 缓冲区
layout(binding = 0) uniform CameraUBO {
    mat4 u_ViewProjection;       // 视图投影矩阵
    mat4 u_View;                // 视图矩阵
    mat4 u_Projection;           // 投影矩阵
};

// 精灵 Uniform 缓冲区
layout(binding = 1) uniform SpriteUBO {
    vec4 u_Color;                // 颜色调制
    float u_Opacity;              // 透明度
    bool u_FlipX;                // X 轴翻转
    bool u_FlipY;                // Y 轴翻转
    bool u_UseColor;              // 是否使用颜色调制
};

// ========== 输出到片段着色器 ==========

layout(location = 0) out vec2 v_TexCoord;     // 纹理坐标
layout(location = 1) out vec4 v_Color;         // 顶点颜色

// ========== 主函数 ==========

void main() {
    // 计算世界位置
    vec4 worldPos = vec4(a_Position, 0.0, 1.0);

    // 应用视图投影矩阵
    gl_Position = u_ViewProjection * worldPos;

    // 输出纹理坐标
    v_TexCoord = a_TexCoord;

    // 输出顶点颜色
    v_Color = a_Color;
}
