#version 450

// UI 顶点着色器
// 用于渲染 2D UI 组件（按钮、图像、文本等）

// 顶点输入
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec4 inColor;

// 输出到片段着色器
layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragColor;

void main() {
    // 直接传递 UV 和颜色
    fragUV = inUV;
    fragColor = inColor;

    // 输出位置（已经是归一化设备坐标或屏幕坐标）
    gl_Position = vec4(inPosition, 0.0, 1.0);
}
