#version 460
#extension GL_EXT_scalar_block_layout : require

// ============================================================================
// PrismaEngine — Water Vertex Shader
// Gerstner wave displacement in vertex shader
// ============================================================================

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in vec3 a_Tangent;

// Camera uniforms
layout(std140, binding = 0) uniform CameraUniforms {
    mat4  u_ViewProjection;
    mat4  u_View;
    mat4  u_Projection;
    vec3  u_CameraPos;
    float u_Near;
    vec3  u_CameraForward;
    float u_Far;
} camera;

// Water uniforms
layout(std140, binding = 1) uniform WaterUniforms {
    vec4  u_DeepColor;        // rgb = deep color, a = unused
    vec4  u_ShallowColor;     // rgb = shallow color, a = unused
    vec4  u_FogColor;         // rgb = fog color, a = density
    float u_WaterLevel;
    float u_WaveHeightScale;
    float u_FresnelPower;
    float u_FresnelStrength;
    float u_SpecularStrength;
    float u_SpecularPower;
    float u_Transparency;
    float u_RefractionScale;
    float u_SunGlowStrength;
    float u_SunGlowPower;
    float u_Time;
    float u_FogDensity;
    float u_ShallowDepth;
    float u_DeepDepth;
} water;

// Gerstner wave data (max 16 waves)
layout(std430, binding = 2) buffer GerstnerBuffer {
    vec4  waveData[16];  // x = dirX, y = dirZ, z = amplitude, w = frequency
    vec4  wavePhase[16]; // x = speed, y = steepness, z = unused, w = unused
} waves;

// Output to fragment shader
layout(location = 0) out vec3 v_WorldPos;
layout(location = 1) out vec3 v_Normal;
layout(location = 2) out vec2 v_TexCoord;
layout(location = 3) out vec3 v_Tangent;
layout(location = 4) out vec3 v_Bitangent;
layout(location = 5) out vec3 v_ViewDir;
layout(location = 6) out float v_WaveHeight;
layout(location = 7) flat out float v_FarPlane;

// Compute Gerstner wave displacement and normal
void computeGerstner(vec3 position, float time,
                     out vec3 displacedPos, out vec3 normal,
                     out vec3 tangent, out vec3 bitangent)
{
    displacedPos = position;
    tangent = vec3(1.0, 0.0, 0.0);
    bitangent = vec3(0.0, 0.0, 1.0);

    for (int i = 0; i < 16; i++) {
        vec4 wd = waves.waveData[i];
        vec4 wp = waves.wavePhase[i];
        if (wd.z < 0.001) continue;

        float dirX = wd.x;
        float dirZ = wd.y;
        float amplitude = wd.z;
        float frequency = wd.w;
        float speed = wp.x;
        float steepness = wp.y;

        float k = (frequency * frequency) / 9.81;
        if (k < 0.001) k = 0.001;

        float Qi = steepness / (k * amplitude);
        Qi = min(Qi, 1.0);

        float theta = k * (dirX * position.x + dirZ * position.z) + speed * time;
        float cosTheta = cos(theta);
        float sinTheta = sin(theta);

        float S = Qi * amplitude * cosTheta;
        displacedPos.x += dirX * S;
        displacedPos.z += dirZ * S;
        displacedPos.y += amplitude * sinTheta;

        float WA = k * amplitude;
        float derivX = dirX * (WA * cosTheta - Qi * WA * sinTheta);
        float derivZ = dirZ * (WA * cosTheta - Qi * WA * sinTheta);
        float derivY = WA * cosTheta;

        tangent.x += dirX * dirX * WA * cosTheta - Qi * WA * sinTheta * dirX * dirX;
        tangent.z += dirX * dirZ * WA * cosTheta - Qi * WA * sinTheta * dirX * dirZ;
        tangent.y += dirX * derivY;

        bitangent.x += dirZ * dirX * WA * cosTheta - Qi * WA * sinTheta * dirZ * dirX;
        bitangent.z += dirZ * dirZ * WA * cosTheta - Qi * WA * sinTheta * dirZ * dirZ;
        bitangent.y += dirZ * derivY;
    }

    normal = normalize(cross(bitangent, tangent));
}

void main() {
    // Original world position (flat water surface)
    vec3 worldPos = vec3(a_Position.x, water.u_WaterLevel, a_Position.z);

    // Apply wave displacement
    vec3 displacedPos;
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
    computeGerstner(worldPos, water.u_Time, displacedPos, normal, tangent, bitangent);

    // Scale wave height
    displacedPos.y = water.u_WaterLevel + (displacedPos.y - water.u_WaterLevel) * water.u_WaveHeightScale;

    // Output to fragment shader
    v_WorldPos = displacedPos;
    v_Normal = normal;
    v_TexCoord = a_TexCoord;
    v_Tangent = tangent;
    v_Bitangent = bitangent;
    v_ViewDir = camera.u_CameraPos - displacedPos;
    v_WaveHeight = displacedPos.y - water.u_WaterLevel;
    v_FarPlane = camera.u_Far;

    gl_Position = camera.u_ViewProjection * vec4(displacedPos, 1.0);
}
