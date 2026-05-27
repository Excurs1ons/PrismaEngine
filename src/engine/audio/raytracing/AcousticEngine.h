#pragma once

#include "AcousticRay.h"
#include <Engine/audio/dsp/AudioNode.h>
#include <Engine/audio/dsp/AudioBuffer.h>
#include <Engine/physics/CollisionSystem.h>
#include <vector>
#include <random>

namespace Prisma::Audio::Raytracing {

class AcousticEngine {
public:
    AcousticEngine() : m_rng(42) {}

    struct AcousticResult {
        std::vector<AcousticRay> paths;
        DirectSound direct;
        struct Reflection { float delay; float attenuation; glm::vec3 direction; };
        std::vector<Reflection> earlyReflections;
        float reverbTailEnergy = 0.0f;
        float reverbTailDuration = 0.0f;
        float occlusionFactor = 0.0f;
    };

    struct DirectSound {
        float gain = 1.0f;
        float delay = 0.0f;
        bool occluded = false;
    };

    void SetGeometry(const std::vector<Physics::AABB>& triangles) {
        m_geometry = triangles;
    }

    AcousticResult Trace(const glm::vec3& source, const glm::vec3& listener,
                         int numRays, float maxDistance) {
        AcousticResult result;

        DirectSound direct;
        direct.delay = glm::distance(source, listener) / 343.0f;
        direct.occluded = IsOccluded(source, listener);
        direct.gain = direct.occluded ? 0.3f : 1.0f;
        result.direct = direct;
        result.occlusionFactor = direct.occluded ? 1.0f : 0.0f;

        for (int i = 0; i < numRays; ++i) {
            AcousticRay ray;
            ray.origin = source;
            ray.direction = RandomDirection();
            ray.maxLength = maxDistance;
            ray.maxBounces = 8;

            float totalDelay = 0.0f;

            while (ray.IsAlive()) {
                RayIntersection hit = TraceRay(ray);
                if (!hit.hit) break;

                float hitDelay = hit.distance / 343.0f;
                totalDelay += hitDelay;
                ray.Advance(hit.distance);
                ray.Attenuate(hit.absorption);
                ray.bounceCount++;

                glm::vec3 toListener = listener - (ray.origin + ray.direction * hit.distance);
                float distToListener = glm::length(toListener);

                if (distToListener < maxDistance && ray.energy > 0.01f) {
                    float reflDelay = totalDelay + distToListener / 343.0f;
                    result.earlyReflections.push_back({
                        reflDelay,
                        ray.energy / (1.0f + distToListener),
                        glm::normalize(toListener)
                    });
                }

                glm::vec3 reflectDir = ray.direction - 2.0f * glm::dot(ray.direction, hit.normal) * hit.normal;
                ray.direction = (hit.scattering > m_scatterDist(m_rng))
                    ? RandomDirection() : reflectDir;
                ray.origin = hit.point + ray.direction * 0.01f;
            }

            result.paths.push_back(ray);
        }

        // Sort early reflections by delay
        std::sort(result.earlyReflections.begin(), result.earlyReflections.end(),
            [](auto& a, auto& b) { return a.delay < b.delay; });

        // Calculate reverb tail from late rays
        float totalEnergy = 0.0f;
        for (auto& p : result.paths) totalEnergy += p.energy;
        result.reverbTailEnergy = totalEnergy / numRays;
        result.reverbTailDuration = maxDistance / 343.0f * 2.0f;

        return result;
    }

    // Apply acoustic results to DSP IR
    void GenerateIR(const AcousticResult& result, std::vector<float>& ir, uint32_t sampleRate) {
        size_t irLength = static_cast<size_t>(result.reverbTailDuration * sampleRate) + 1024;
        ir.assign(irLength, 0.0f);

        // Direct sound
        if (!result.direct.occluded) {
            size_t directIdx = static_cast<size_t>(result.direct.delay * sampleRate);
            if (directIdx < irLength) ir[directIdx] = result.direct.gain;
        }

        // Early reflections
        for (auto& refl : result.earlyReflections) {
            size_t idx = static_cast<size_t>(refl.delay * sampleRate);
            if (idx < irLength) ir[idx] += refl.attenuation;
        }

        // Late reverb tail (exponential decay)
        size_t earlyEnd = result.earlyReflections.empty() ? 0 :
            static_cast<size_t>(result.earlyReflections.back().delay * sampleRate);
        for (size_t i = earlyEnd; i < irLength; ++i) {
            float t = static_cast<float>(i) / sampleRate;
            float decay = expf(-t * 3.0f / result.reverbTailDuration);
            ir[i] += result.reverbTailEnergy * decay * 0.1f;
        }
    }

private:
    bool IsOccluded(const glm::vec3& from, const glm::vec3& to) {
        glm::vec3 dir = to - from;
        float dist = glm::length(dir);
        dir /= dist;

        RayIntersection closest;
        closest.distance = dist;

        for (auto& tri : m_geometry) {
            Physics::Ray ray(glm::vec3(from.x, from.y, from.z), glm::vec3(dir.x, dir.y, dir.z));
            auto hit = Physics::CollisionSystem::rayCastAABB(ray, tri);
            if (hit.hit && hit.distance < dist - 0.1f) return true;
        }
        return false;
    }

    RayIntersection TraceRay(const AcousticRay& ray) {
        RayIntersection closest;
        closest.distance = ray.maxLength;

        Physics::Ray pRay(
            glm::vec3(ray.origin.x, ray.origin.y, ray.origin.z),
            glm::vec3(ray.direction.x, ray.direction.y, ray.direction.z)
        );

        for (auto& tri : m_geometry) {
            auto hit = Physics::CollisionSystem::rayCastAABB(pRay, tri);
            if (hit.hit && hit.distance < closest.distance && hit.distance > 0.01f) {
                closest.hit = true;
                closest.distance = hit.distance;
                closest.point = ray.origin + ray.direction * hit.distance;
                closest.normal = hit.normal;
                closest.absorption = 0.5f;
            }
        }
        return closest;
    }

    glm::vec3 RandomDirection() {
        float theta = 2.0f * M_PI * m_dist(m_rng);
        float phi = acosf(2.0f * m_dist(m_rng) - 1.0f);
        return glm::vec3(
            sinf(phi) * cosf(theta),
            sinf(phi) * sinf(theta),
            cosf(phi)
        );
    }

    std::vector<Physics::AABB> m_geometry;
    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_dist{0.0f, 1.0f};
    std::uniform_real_distribution<float> m_scatterDist{0.0f, 1.0f};
};

} // namespace Prisma::Audio::Raytracing
