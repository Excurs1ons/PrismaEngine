#version 450
layout(location = 0) in vec4 v_Color;
layout(location = 1) in vec2 v_UV;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D AlbedoMap;
layout(binding = 1) uniform sampler2D LightMap;

void main() {
    vec4 texColor = texture(AlbedoMap, v_UV);
    vec4 lightColor = texture(LightMap, gl_FragCoord.xy / textureSize(LightMap, 0));
    outColor = v_Color * texColor * vec4(lightColor.rgb, 1.0);
}
