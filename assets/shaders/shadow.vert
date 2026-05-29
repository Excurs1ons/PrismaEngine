// ============================================================
// shadow.vert - PrismaEngine 阴影深度顶点着色器
//
// 从光源视角渲染深度，用于级联阴影映射 (CSM)
// Push Constant: 光源视投影矩阵 (CPU 预乘世界矩阵)
// ============================================================

#version 450 core

layout(location = 0) in vec3 inPosition;

layout(push_constant) uniform ShadowPC {
    mat4 lightVP;
} pc;

void main() {
    gl_Position = pc.lightVP * vec4(inPosition, 1.0);
}
