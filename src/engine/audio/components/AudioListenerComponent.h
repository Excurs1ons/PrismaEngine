#pragma once

#include <cstdint>

namespace Prisma::Audio::Components {

struct AudioListenerComponent {
    bool isActive = true;
    float gain = 1.0f;
};

} // namespace Prisma::Audio::Components
