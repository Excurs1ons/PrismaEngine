#version 450

// 输入顶点属性
layout(location = 0) in vec2 a_Position;
layout(location = 1) in vec2 a_TexCoord;
layout(location = 2) in float a_TexIndex;
layout(location = 3) in vec4 a_Color;

// Uniform 缓冲区
layout(binding = 0) uniform CameraUBO {
    mat4 u_ViewProjection;
    mat4 u_View;
    mat4 u_Projection;
};

// 输出到片段着色器
layout(location = 0) out vec2 v_TexCoord;
layout(location = 1) out float v_TexIndex;
layout(location = 2) out vec4 v_Color;

void main() {
    vec4 worldPos = vec4(a_Position, 0.0, 1.0);
    gl_Position = u_ViewProjection * worldPos;

    v_TexCoord = a_TexCoord;
    v_TexIndex = a_TexIndex;
    v_Color = a_Color;
}
