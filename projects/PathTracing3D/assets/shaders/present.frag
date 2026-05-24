#version 450
// Sample path tracing output and present to swapchain
// Also applies tone mapping (gamma correction)
layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D presentTexture;

void main() {
    vec4 color = texture(presentTexture, inUV);
    
    // Reinhard tone mapping
    vec3 mapped = color.rgb / (color.rgb + vec3(1.0));
    
    // Gamma correction
    outColor = vec4(pow(mapped, vec3(1.0 / 2.2)), 1.0);
}
