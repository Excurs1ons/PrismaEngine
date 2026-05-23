#version 450
layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;
layout(binding = 0) uniform sampler2D lightingTexture;
layout(binding = 1) uniform sampler2D ssgiTexture;
void main() {
    vec3 color = texture(lightingTexture, inUV).rgb;
    color += texture(ssgiTexture, inUV).rgb;
    outColor = vec4(color, 1.0);
}
