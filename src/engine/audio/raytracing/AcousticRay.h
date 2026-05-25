#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

namespace Prisma::Audio::Raytracing {

struct AcousticRay {
    glm::vec3 origin;
    glm::vec3 direction;
    float energy = 1.0f;
    float length = 0.0f;
    float maxLength = 50.0f;
    int bounceCount = 0;
    int maxBounces = 8;
    float frequency = 1000.0f;

    bool IsAlive() const { return energy > 0.001f && length < maxLength && bounceCount < maxBounces; }
    void Advance(float distance) { length += distance; }
    void Attenuate(float factor) { energy *= factor; energy = std::max(0.0f, energy); }
};

struct RayIntersection {
    bool hit = false;
    float distance = 0.0f;
    glm::vec3 point{0.0f};
    glm::vec3 normal{0.0f};
    uint32_t materialIndex = 0;
    float absorption = 0.5f;
    float scattering = 0.1f;
};

struct AcousticMaterial {
    float absorption = 0.5f;
    float scattering = 0.1f;
    float transmission = 0.0f;
};

} // namespace Prisma::Audio::Raytracing
