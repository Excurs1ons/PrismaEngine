#include "water/WaterInteraction.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace Prisma::Water {

void WaterInteraction::SpawnRipple(const Vector3& position,
                                    float strength,
                                    float maxRadius,
                                    float lifetime)
{
    Ripple r;
    r.center = position;
    r.startRadius = 0.0f;
    r.currentRadius = 0.0f;
    r.maxRadius = maxRadius;
    r.amplitude = std::clamp(strength, 0.1f, 10.0f);
    r.lifetime = lifetime;
    r.age = 0.0f;

    if (m_Ripples.size() >= m_MaxRipples) {
        float oldestAge = -1.0f;
        size_t oldestIdx = 0;
        for (size_t i = 0; i < m_Ripples.size(); ++i) {
            if (m_Ripples[i].GetNormalizedAge() > oldestAge) {
                oldestAge = m_Ripples[i].GetNormalizedAge();
                oldestIdx = i;
            }
        }
        m_Ripples[oldestIdx] = r;
    } else {
        m_Ripples.push_back(r);
    }
}

void WaterInteraction::Update(float deltaTime, float rippleSpeed, float decayRate)
{
    for (auto& ripple : m_Ripples) {
        ripple.age += deltaTime;
        ripple.currentRadius = ripple.startRadius + rippleSpeed * ripple.age;
        ripple.amplitude *= (1.0f - decayRate * deltaTime);
        if (ripple.currentRadius > ripple.maxRadius) {
            ripple.currentRadius = ripple.maxRadius;
        }
    }

    auto it = std::remove_if(m_Ripples.begin(), m_Ripples.end(),
        [](const Ripple& r) { return !r.IsAlive(); });
    m_Ripples.erase(it, m_Ripples.end());
}

float WaterInteraction::GetDisplacement(const Vector2& worldPos) const
{
    float displacement = 0.0f;

    for (const auto& ripple : m_Ripples) {
        // World position to ripple center distance
        float dx = worldPos.x - ripple.center.x;
        float dz = worldPos.y - ripple.center.z; // worldPos.y is Z in 3D
        float dist = std::sqrt(dx * dx + dz * dz);

        // Ripple ring: sinusoidal displacement that fades with distance from center
        float ringDist = std::abs(dist - ripple.currentRadius);

        // Ring width: proportional to ripple size, min 0.5m
        float ringWidth = std::max(0.5f, ripple.currentRadius * 0.15f);

        // Sine wave with gaussian falloff from ring center
        float ringFactor = std::exp(-(ringDist * ringDist) / (2.0f * ringWidth * ringWidth));
        float ringWave = std::sin(dist * 0.5f - ripple.currentRadius * 0.5f);

        // Composite displacement
        displacement += ripple.amplitude * ringFactor * ringWave * ripple.GetFade();
    }

    return displacement;
}

void WaterInteraction::Clear()
{
    m_Ripples.clear();
}

} // namespace Prisma::Water
