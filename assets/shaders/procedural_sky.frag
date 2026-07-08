#version 450

// 程序化物理天空 fragment shader
// Preetham 风格 Rayleigh + Mie 单次散射近似

layout(location = 0) in vec3 vDir;
layout(location = 0) out vec4 FragColor;

// 单个 UBO (binding 0),vert/frag 共享,与 ProceduralSkyPass::SkyUBO 内存布局一致
layout(set = 0, binding = 0) uniform SkyUBO {
    mat4 viewProj;
    vec4 sunDirIntensity;  // xyz=dir, w=intensity
    vec4 sunColor;         // rgb=color, a=pad
};

// Rayleigh/Mie 系数 (Preetham 近似)
const vec3 rayleighCoeff = vec3(5.8e-6, 13.5e-6, 33.1e-6) * 1.0;
const float mieCoeff = 21e-6;
const float rayleighScaleHeight = 8.4e3;   // 米
const float mieScaleHeight = 1.2e3;

// 相位函数
float rayleighPhase(float cosTheta) {
    return (3.0 / (16.0 * 3.14159265)) * (1.0 + cosTheta * cosTheta);
}
float miePhase(float cosTheta, float g) {
    float g2 = g * g;
    return (3.0 / (8.0 * 3.14159265)) * ((1.0 - g2) * (1.0 + cosTheta * cosTheta))
         / ((2.0 + g2) * pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5));
}

void main() {
    vec3 dir = normalize(vDir);
    vec3 sunDir = normalize(sunDirIntensity.xyz);
    float sunIntensity = sunDirIntensity.w;
    vec3 sunCol = sunColor.rgb;

    // 简化:假设观察方向穿过整层大气(地面观察)
    float cosTheta = dot(dir, sunDir);

    // Rayleigh 散射
    float rayleighOptical = 1.0;  // 简化:整层
    vec3 rayleighScatter = rayleighCoeff * rayleighPhase(cosTheta);
    // 衰减
    vec3 rayleighTransmit = exp(-(rayleighCoeff * 1.0));

    // Mie 散射
    float mieOptical = 1.0;
    float g = 0.76;
    float mieScatter = mieCoeff * miePhase(cosTheta, g);
    float mieTransmit = exp(-(mieCoeff * 1.0));

    // 天空颜色 = 散射光 * 透射
    vec3 skyColor = (rayleighScatter * rayleighTransmit + mieScatter * mieTransmit) * sunCol * sunIntensity;

    // 太阳圆盘(强光晕)
    float sunDisk = smoothstep(0.9995, 0.9999, cosTheta);
    skyColor += sunCol * sunIntensity * sunDisk * 200.0;

    // 地平线增亮(简化)
    float horizon = 1.0 - abs(dir.y);
    skyColor += vec3(0.3, 0.2, 0.1) * horizon * horizon * 0.3;

    // 最低亮度保底
    skyColor = max(skyColor, vec3(0.01, 0.02, 0.05));

    FragColor = vec4(skyColor, 1.0);
}
