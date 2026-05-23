#version 450

layout(location = 0) in vec3 v_WorldPos;
layout(location = 1) in vec4 v_Color;
layout(location = 2) in vec3 v_Normal;
layout(location = 3) in vec2 v_UV;
layout(location = 4) in vec3 v_ViewPos;

layout(location = 0) out vec4 outColor;

struct Light {
    vec4 position; // w: padding
    vec4 color;    // w: range
    vec4 direction; // w: type
};

// Set 0: Material
layout(set = 0, binding = 0, std140) uniform MaterialData {
    vec4 baseColor;
    float metallic;
    float roughness;
} material;

layout(set = 0, binding = 1) uniform sampler2D albedoMap;

// Set 2: Clustered Data
layout(set = 2, binding = 0, std430) readonly buffer LightBuffer {
    Light lights[];
};

layout(set = 2, binding = 1, std430) readonly buffer GlobalLightIndexList {
    uint globalIndexList[];
};

layout(set = 2, binding = 2, std430) readonly buffer ClusterLightGrid {
    uvec2 clusterGrid[]; // offset, count
};

layout(set = 2, binding = 3) uniform CameraData {
    mat4 projection;
    mat4 invProjection;
    mat4 view;
    float nearPlane;
    float farPlane;
    uvec2 gridSize;
    uvec2 screenSize;
    uint totalLights;
    uint numZSlices;
} camera;

void main() {
    // 1. Cluster Calculation
    uvec2 tileID = uvec2(gl_FragCoord.xy / (vec2(camera.screenSize) / vec2(camera.gridSize.xy)));
    float zView = max(v_ViewPos.z, camera.nearPlane); 
    float denom = log(camera.farPlane / camera.nearPlane);
    uint zSlice = uint(max(0.0, log(zView / camera.nearPlane) * float(camera.numZSlices) / denom));
    zSlice = min(zSlice, camera.numZSlices - 1);

    uint clusterIndex = tileID.x + 
                        camera.gridSize.x * (tileID.y + 
                        camera.gridSize.y * zSlice);

    uvec2 gridData = clusterGrid[clusterIndex];
    uint offset = gridData.x;
    uint count = gridData.y;

    // 2. Standard Lighting
    vec3 N = normalize(v_Normal);
    
    // 使用 1x1 白色纹理兜底的采样逻辑
    vec3 albedo = material.baseColor.rgb * texture(albedoMap, v_UV).rgb;
    
    vec3 ambient = albedo * 0.1;
    vec3 totalDiffuse = vec3(0.0);

    for (uint i = 0; i < count; i++) {
        uint lightIndex = globalIndexList[offset + i];
        if (lightIndex >= 1024) continue;

        Light light = lights[lightIndex];
        int type = int(light.direction.w);

        if (type == 0) { // Directional
            vec3 L = normalize(-light.direction.xyz);
            totalDiffuse += max(dot(N, L), 0.0) * light.color.rgb;
        } else if (type == 1) { // Point
            vec3 lightDir = light.position.xyz - v_WorldPos;
            float dist = length(lightDir);
            float range = light.color.w;
            if (dist < range) {
                vec3 L = normalize(lightDir);
                float attenuation = pow(max(0.0, 1.0 - (dist / range)), 2.0);
                totalDiffuse += max(dot(N, L), 0.0) * light.color.rgb * attenuation;
            }
        }
    }

    vec3 finalColor = ambient + totalDiffuse;
    
    // Tone Mapping & Gamma
    finalColor = finalColor / (finalColor + vec3(1.0));
    finalColor = pow(finalColor, vec3(1.0 / 2.2));

    outColor = vec4(finalColor, 1.0);
}
