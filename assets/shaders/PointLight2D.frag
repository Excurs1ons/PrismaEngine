#version 450
layout(location = 0) in vec2 v_TexCoord;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform LightPushConstants {
    vec2 u_Position;    // World Position
    vec3 u_Color;
    float u_Intensity;
    float u_Radius;
    float u_Falloff;
    mat4 u_InvVP;       // Inverse View-Projection matrix to get world pos from tex coord
} pc;

void main() {
    // Convert TexCoord (0..1) to NDC (-1..1)
    vec4 ndc = vec4(v_TexCoord.x * 2.0 - 1.0, (1.0 - v_TexCoord.y) * 2.0 - 1.0, 0.0, 1.0);
    // Transform to world space
    vec4 worldPos4 = pc.u_InvVP * ndc;
    vec2 worldPos = worldPos4.xy / worldPos4.w;
    
    float dist = distance(worldPos, pc.u_Position);
    
    if (dist > pc.u_Radius) {
        discard;
    }
    
    float normDist = dist / pc.u_Radius;
    float atten = pow(max(0.0, 1.0 - normDist), pc.u_Falloff);
    
    outColor = vec4(pc.u_Color * pc.u_Intensity * atten, 1.0);
}
