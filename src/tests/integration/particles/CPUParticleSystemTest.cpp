#include <gtest/gtest.h>
#include "particles/CPUParticleSystem.h"

namespace Prisma::Particles {
namespace {

// ============================================================================
// Particle — Basic Properties
// ============================================================================
TEST(ParticleTest, DefaultState) {
    Particle p;
    EXPECT_EQ(p.position, Vector3(0.0f));
    EXPECT_EQ(p.velocity, Vector3(0.0f));
    EXPECT_EQ(p.color, Vector4(1.0f));
    EXPECT_FLOAT_EQ(p.size, 1.0f);
    EXPECT_FLOAT_EQ(p.lifetime, 1.0f);
    EXPECT_FLOAT_EQ(p.age, 0.0f);
    EXPECT_FLOAT_EQ(p.rotation, 0.0f);
    EXPECT_FLOAT_EQ(p.angularVelocity, 0.0f);
}

TEST(ParticleTest, IsAlive) {
    Particle p;
    p.lifetime = 2.0f;
    p.age = 0.0f;
    EXPECT_TRUE(p.IsAlive());

    p.age = 1.0f;
    EXPECT_TRUE(p.IsAlive());

    p.age = 2.0f;
    EXPECT_FALSE(p.IsAlive());

    p.age = 3.0f;
    EXPECT_FALSE(p.IsAlive());
}

TEST(ParticleTest, GetNormalizedAge) {
    Particle p;
    p.lifetime = 4.0f;

    p.age = 0.0f;
    EXPECT_FLOAT_EQ(p.GetNormalizedAge(), 0.0f);

    p.age = 2.0f;
    EXPECT_FLOAT_EQ(p.GetNormalizedAge(), 0.5f);

    p.age = 4.0f;
    EXPECT_FLOAT_EQ(p.GetNormalizedAge(), 1.0f);

    // Clamped
    p.age = 8.0f;
    EXPECT_FLOAT_EQ(p.GetNormalizedAge(), 1.0f);
}

// ============================================================================
// CPUParticleSystem — Default State
// ============================================================================
class CPUParticleSystemTest : public ::testing::Test {
protected:
    CPUParticleSystem m_system;
    std::mt19937 m_rng{42};
    EmitterConfig m_defaultConfig;

    void SetUp() override {
        m_defaultConfig.spawnRate = 10.0f;
        m_defaultConfig.lifetime = Range<float>(2.0f, 2.0f);
        m_defaultConfig.speed = Range<float>(1.0f, 1.0f);
        m_defaultConfig.shape = EmitterShape::Point;
        m_defaultConfig.gravity = Vector3(0.0f, -9.81f, 0.0f);
    }
};

TEST_F(CPUParticleSystemTest, InitiallyEmpty) {
    EXPECT_EQ(m_system.GetAliveCount(), 0u);
    EXPECT_EQ(m_system.GetTotalCount(), 0u);
    EXPECT_EQ(m_system.GetCapacity(), 10000u);
}

TEST_F(CPUParticleSystemTest, SetCapacity) {
    m_system.SetCapacity(500);
    EXPECT_EQ(m_system.GetCapacity(), 500u);
}

TEST_F(CPUParticleSystemTest, SortEnabledByDefault) {
    EXPECT_TRUE(m_system.IsSortEnabled());
    m_system.SetSortEnabled(false);
    EXPECT_FALSE(m_system.IsSortEnabled());
}

// ============================================================================
// CPUParticleSystem — Spawn
// ============================================================================
TEST_F(CPUParticleSystemTest, SpawnParticles) {
    m_system.Spawn(10, m_defaultConfig, m_rng);
    EXPECT_EQ(m_system.GetTotalCount(), 10u);
    EXPECT_EQ(m_system.GetAliveCount(), 10u);

    const auto& particles = m_system.GetParticles();
    ASSERT_EQ(particles.size(), 10u);
    for (const auto& p : particles) {
        EXPECT_EQ(p.position, Vector3(0.0f));
        EXPECT_EQ(p.lifetime, 2.0f);
        EXPECT_FLOAT_EQ(p.age, 0.0f);
    }
}

TEST_F(CPUParticleSystemTest, SpawnBurst) {
    m_defaultConfig.burstCount = 25;
    m_system.SpawnBurst(m_defaultConfig, m_rng);
    EXPECT_EQ(m_system.GetTotalCount(), 25u);
    EXPECT_EQ(m_system.GetAliveCount(), 25u);
}

TEST_F(CPUParticleSystemTest, SpawnWithSphereShape) {
    m_defaultConfig.shape = EmitterShape::Sphere;
    m_defaultConfig.radius = 5.0f;

    m_system.Spawn(100, m_defaultConfig, m_rng);
    EXPECT_EQ(m_system.GetTotalCount(), 100u);

    const auto& particles = m_system.GetParticles();
    for (const auto& p : particles) {
        float dist = glm::length(p.position);
        EXPECT_LE(dist, 5.0f + 0.01f);
    }
}

TEST_F(CPUParticleSystemTest, SpawnWithBoxShape) {
    m_defaultConfig.shape = EmitterShape::Box;
    m_defaultConfig.boxExtents = Vector3(2.0f, 3.0f, 4.0f);

    m_system.Spawn(100, m_defaultConfig, m_rng);
    const auto& particles = m_system.GetParticles();
    for (const auto& p : particles) {
        EXPECT_GE(p.position.x, -2.0f);
        EXPECT_LE(p.position.x, 2.0f);
        EXPECT_GE(p.position.y, -3.0f);
        EXPECT_LE(p.position.y, 3.0f);
        EXPECT_GE(p.position.z, -4.0f);
        EXPECT_LE(p.position.z, 4.0f);
    }
}

// ============================================================================
// CPUParticleSystem — Update (position, velocity, lifetime)
// ============================================================================
TEST_F(CPUParticleSystemTest, UpdateMovesParticles) {
    m_system.Spawn(5, m_defaultConfig, m_rng);

    Vector3 cameraPos(0.0f, 0.0f, 10.0f);
    m_system.Update(0.5f, cameraPos);

    const auto& particles = m_system.GetParticles();
    EXPECT_EQ(particles.size(), 5u);
    for (const auto& p : particles) {
        EXPECT_FLOAT_EQ(p.age, 0.5f);
    }
}

TEST_F(CPUParticleSystemTest, UpdateRemovesExpiredParticles) {
    EmitterConfig cfg;
    cfg.lifetime = Range<float>(0.01f, 0.01f);
    cfg.speed = Range<float>(0.0f, 0.0f);
    cfg.shape = EmitterShape::Point;

    m_system.Spawn(10, cfg, m_rng);
    EXPECT_EQ(m_system.GetAliveCount(), 10u);

    Vector3 cameraPos(0.0f, 0.0f, 10.0f);
    m_system.Update(0.1f, cameraPos);

    EXPECT_EQ(m_system.GetAliveCount(), 0u);
    EXPECT_EQ(m_system.GetTotalCount(), 0u);
}

TEST_F(CPUParticleSystemTest, UpdateIntegratesPosition) {
    EmitterConfig cfg;
    cfg.lifetime = Range<float>(10.0f, 10.0f);
    cfg.speed = Range<float>(5.0f, 5.0f);
    cfg.shape = EmitterShape::Point;

    m_system.Spawn(1, cfg, m_rng);
    Vector3 cameraPos(0.0f, 0.0f, 10.0f);

    const auto& before = m_system.GetParticles();
    Vector3 startPos = before[0].position;
    Vector3 startVel = before[0].velocity;

    m_system.Update(1.0f, cameraPos);

    const auto& after = m_system.GetParticles();
    ASSERT_EQ(after.size(), 1u);
    Vector3 expectedPos = startPos + startVel * 1.0f;
    EXPECT_FLOAT_EQ(after[0].position.x, expectedPos.x);
    EXPECT_FLOAT_EQ(after[0].position.y, expectedPos.y);
    EXPECT_FLOAT_EQ(after[0].position.z, expectedPos.z);
    EXPECT_FLOAT_EQ(after[0].velocity.x, startVel.x);
    EXPECT_FLOAT_EQ(after[0].velocity.y, startVel.y);
    EXPECT_FLOAT_EQ(after[0].velocity.z, startVel.z);
}

// ============================================================================
// CPUParticleSystem — Clear / Reset
// ============================================================================
TEST_F(CPUParticleSystemTest, ClearRemovesAllParticles) {
    m_system.Spawn(20, m_defaultConfig, m_rng);
    EXPECT_EQ(m_system.GetTotalCount(), 20u);

    m_system.Clear();
    EXPECT_EQ(m_system.GetTotalCount(), 0u);
    EXPECT_EQ(m_system.GetAliveCount(), 0u);
}

TEST_F(CPUParticleSystemTest, ResetAfterUpdate) {
    m_system.Spawn(5, m_defaultConfig, m_rng);
    Vector3 cameraPos(0.0f, 0.0f, 10.0f);
    m_system.Update(1.0f, cameraPos);

    m_system.Reset();
    EXPECT_EQ(m_system.GetTotalCount(), 0u);
    EXPECT_EQ(m_system.GetAliveCount(), 0u);
    EXPECT_TRUE(m_system.IsSortEnabled());
    EXPECT_EQ(m_system.GetLodFactor(), 1.0f);
}

// ============================================================================
// CPUParticleSystem — Sort By Distance
// ============================================================================
TEST_F(CPUParticleSystemTest, SortByDistance) {
    m_system.SetSortEnabled(true);

    m_system.Spawn(3, m_defaultConfig, m_rng);
    auto& particles = m_system.GetParticles();
    ASSERT_GE(particles.size(), 3u);

    particles[0].position = Vector3(0.0f, 0.0f, 0.0f);
    particles[1].position = Vector3(10.0f, 0.0f, 0.0f);
    particles[2].position = Vector3(5.0f, 0.0f, 0.0f);

    Vector3 cameraPos(0.0f, 0.0f, 0.0f);
    m_system.SortByDistance(cameraPos);

    EXPECT_GE(glm::distance(particles[0].position, cameraPos),
              glm::distance(particles[1].position, cameraPos));
    EXPECT_GE(glm::distance(particles[1].position, cameraPos),
              glm::distance(particles[2].position, cameraPos));
}

// ============================================================================
// CPUParticleSystem — LOD
// ============================================================================
TEST_F(CPUParticleSystemTest, LodFactor) {
    EXPECT_FLOAT_EQ(m_system.GetLodFactor(), 1.0f);
    m_system.SetLodFactor(0.5f);
    EXPECT_FLOAT_EQ(m_system.GetLodFactor(), 0.5f);
    m_system.SetLodFactor(2.0f);
    EXPECT_FLOAT_EQ(m_system.GetLodFactor(), 1.0f);
    m_system.SetLodFactor(-1.0f);
    EXPECT_FLOAT_EQ(m_system.GetLodFactor(), 0.0f);
}

// ============================================================================
// Range Utility
// ============================================================================
TEST(RangeTest, DefaultConstructor) {
    Range<float> r;
    EXPECT_FLOAT_EQ(r.min, 0.0f);
    EXPECT_FLOAT_EQ(r.max, 0.0f);
}

TEST(RangeTest, SingleValue) {
    Range<float> r(5.0f);
    EXPECT_FLOAT_EQ(r.min, 5.0f);
    EXPECT_FLOAT_EQ(r.max, 5.0f);
}

TEST(RangeTest, MinMax) {
    Range<float> r(1.0f, 10.0f);
    EXPECT_FLOAT_EQ(r.min, 1.0f);
    EXPECT_FLOAT_EQ(r.max, 10.0f);
}

TEST(RangeTest, Mid) {
    Range<float> r(1.0f, 5.0f);
    EXPECT_FLOAT_EQ(r.Mid(), 3.0f);
}

TEST(RangeTest, RandomInRange) {
    Range<float> r(0.0f, 100.0f);
    std::mt19937 rng{123};
    for (int i = 0; i < 100; ++i) {
        float v = r.Random(rng);
        EXPECT_GE(v, 0.0f);
        EXPECT_LE(v, 100.0f);
    }
}

TEST(RangeTest, Vector3Mid) {
    Range<Vector3> r(Vector3(1.0f, 2.0f, 3.0f), Vector3(3.0f, 4.0f, 5.0f));
    Vector3 mid = r.Mid();
    EXPECT_FLOAT_EQ(mid.x, 2.0f);
    EXPECT_FLOAT_EQ(mid.y, 3.0f);
    EXPECT_FLOAT_EQ(mid.z, 4.0f);
}

TEST(RangeTest, Vector3RandomInRange) {
    Range<Vector3> r(Vector3(-10.0f), Vector3(10.0f));
    std::mt19937 rng{456};
    for (int i = 0; i < 50; ++i) {
        Vector3 v = r.Random(rng);
        EXPECT_GE(v.x, -10.0f);
        EXPECT_LE(v.x, 10.0f);
        EXPECT_GE(v.y, -10.0f);
        EXPECT_LE(v.y, 10.0f);
        EXPECT_GE(v.z, -10.0f);
        EXPECT_LE(v.z, 10.0f);
    }
}

} // namespace
} // namespace Prisma::Particles
