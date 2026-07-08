#version 450

// 程序化物理天空 vertex shader
// 立方体顶点位置作为方向向量传给 frag,移除 viewProj 平移,z=w 保证最远深度

layout(location = 0) in vec4 aPos;

layout(location = 0) out vec3 vDir;

// 单个 UBO (binding 0),vert/frag 共享,与 ProceduralSkyPass::SkyUBO 内存布局一致
layout(set = 0, binding = 0) uniform SkyUBO {
    mat4 viewProj;
    vec4 sunDirIntensity;  // xyz=dir, w=intensity
    vec4 sunColor;         // rgb=color, a=pad
};

void main() {
    vDir = aPos.xyz;
    mat4 vp = viewProj;
    vp[3] = vec4(0.0, 0.0, 0.0, 1.0);  // 移除平移,天空盒跟随相机
    vec4 pos = vp * vec4(aPos.xyz, 1.0);
    gl_Position = pos.xyww;  // z = w -> 深度 1.0 (最远)
}
