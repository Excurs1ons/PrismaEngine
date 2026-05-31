#version 450

layout(location = 0) in vec2 v_TexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform texture2D u_Texture;
layout(binding = 1) uniform sampler u_Sampler;

void main() {
    // 鏈€杩戦偦閲囨牱锛歱oint sampler 纭繚鍍忕礌娓呮櫚鏃犳ā绯?    outColor = texture(sampler2D(u_Texture, u_Sampler), v_TexCoord);
}
