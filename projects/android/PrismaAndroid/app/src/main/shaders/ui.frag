#version 450

// UI 片段着色器
// 用于渲染 2D UI 组件（按钮、图像、文本等）

// 来自顶点着色器的输入
layout(location = 0) in vec2 fragUV;
layout(location = 1) in vec4 fragColor;

// 输出颜色
layout(location = 0) out vec4 outColor;

void main() {
    // 简单的颜色输出
    // TODO: 后续可以添加纹理采样支持
    outColor = fragColor;
}
