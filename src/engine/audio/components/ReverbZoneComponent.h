#pragma once

#include <cstdint>

namespace Prisma::Audio::Components {

struct ReverbZoneComponent {
    float radius = 10.0f;
    float roomSize = 0.7f;
    float damping = 0.5f;
    float width = 1.0f;
    float wetLevel = 0.5f;
    float dryLevel = 1.0f;
    bool isActive = true;
};

} // namespace Prisma::Audio::Components
