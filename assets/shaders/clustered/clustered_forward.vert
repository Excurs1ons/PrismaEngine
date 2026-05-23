#version 450

layout(location = 0) in vec4 inPos;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec4 inUV;
layout(location = 3) in vec4 inNormal;

layout(location = 0) out vec3 v_WorldPos;
layout(location = 1) out vec4 v_Color;
layout(location = 2) out vec3 v_Normal;
layout(location = 3) out vec2 v_UV;
layout(location = 4) out vec3 v_ViewPos;

layout(push_constant) uniform PushConstants {
    mat4 world;
    vec4 color;
} pc;

// Set 1: Scene
layout(set = 1, binding = 0) uniform SceneData {
    mat4 view;
    mat4 projection;
    mat4 viewProjection;
} scene;

void main() {
    vec4 worldPos = pc.world * vec4(inPos.xyz, 1.0);
    gl_Position = scene.viewProjection * worldPos;
    
    v_WorldPos = worldPos.xyz;
    v_Color = inColor * pc.color;
    v_Normal = mat3(transpose(inverse(pc.world))) * inNormal.xyz;
    v_UV = inUV.xy;
    v_ViewPos = (scene.view * worldPos).xyz;
}
