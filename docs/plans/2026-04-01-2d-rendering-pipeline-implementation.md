# 2D 渲染管线实现计划

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** 为 PrismaEngine 实现完整的 2D 渲染管线，包括 Lighting2D 光照系统和 ParticleSystem 粒子系统，为 Pac-Man 游戏提供完整的 2D 渲染能力。

**Architecture:** 扩展现有 Renderer2D 渲染架构，添加 Lighting2DPass 处理 2D 光照，创建 ParticleSystem 处理粒子效果。利用现有的 SpriteRenderer、SpriteAnimation 和 TilemapRenderer 组件，构建完整的 2D 渲染管线。

**Tech Stack:** C++20, OpenGL/Vulkan, GLM, PrismaEngine 现有架构

---

## 已完成功能

以下功能已存在于引擎中，无需重新实现：

- ✅ **SpriteAnimation** (`src/engine/graphic/SpriteAnimation.h/.cpp`) - 精灵动画系统
- ✅ **SpriteRenderer** (`src/engine/graphic/SpriteRenderer.h/.cpp`) - 精灵渲染组件
- ✅ **Renderer2D** (`src/engine/graphic/Renderer2D.h/.cpp`) - 2D 批量渲染 API
- ✅ **TilemapRenderer** (`src/engine/tilemap/renderer/TilemapRenderer.h/.cpp`) - 瓦片地图渲染

---

## 需要实现的功能

### 任务 1: 实现 Lighting2D 光照系统

#### Task 1.1: 创建 Light2D 光源类

**文件:**
- 创建: `src/engine/graphic/2d/Light2D.h`
- 创建: `src/engine/graphic/2d/Light2D.cpp`
- 修改: `src/engine/graphic/CMakeLists.txt` (添加新文件)

**Step 1: 编写失败的测试**

创建测试文件 `tests/graphic/test_light2d.cpp`:

```cpp
#include <gtest/gtest.h>
#include "graphic/2d/Light2D.h"

using namespace Prisma::Graphic;

TEST(Light2DTest, DefaultConstruction) {
    Light2D light;
    
    EXPECT_EQ(light.GetType(), Light2D::Type::Point);
    EXPECT_EQ(light.GetPosition(), Vector2(0.0f, 0.0f));
    EXPECT_EQ(light.GetColor(), Vector3(1.0f, 1.0f, 1.0f));
    EXPECT_FLOAT_EQ(light.GetIntensity(), 1.0f);
    EXPECT_FLOAT_EQ(light.GetRadius(), 100.0f);
    EXPECT_FALSE(light.IsCastShadows());
}

TEST(Light2DTest, SetPropertyUpdates) {
    Light2D light;
    
    light.SetType(Light2D::Type::Spot);
    EXPECT_EQ(light.GetType(), Light2D::Type::Spot);
    
    light.SetPosition({100.0f, 200.0f});
    EXPECT_EQ(light.GetPosition(), Vector2(100.0f, 200.0f));
    
    light.SetColor({1.0f, 0.0f, 0.0f});
    EXPECT_EQ(light.GetColor(), Vector3(1.0f, 0.0f, 0.0f));
    
    light.SetIntensity(2.5f);
    EXPECT_FLOAT_EQ(light.GetIntensity(), 2.5f);
    
    light.SetRadius(150.0f);
    EXPECT_FLOAT_EQ(light.GetRadius(), 150.0f);
    
    light.SetCastShadows(true);
    EXPECT_TRUE(light.IsCastShadows());
}

TEST(Light2DTest, DirectionalLightProperties) {
    Light2D light(Light2D::Type::Directional);
    
    light.SetDirection({1.0f, -1.0f});
    EXPECT_EQ(light.GetDirection(), Vector2(1.0f, -1.0f));
    
    // 方向光应该忽略位置和半径
    light.SetPosition({100.0f, 100.0f});
    light.SetRadius(50.0f);
    
    // 方向光的衰减应该基于方向而非距离
    EXPECT_TRUE(light.IsDirectional());
}

TEST(Light2DTest, SpotLightConeAngle) {
    Light2D light(Light2D::Type::Spot);
    
    light.SetSpotAngle(45.0f);
    EXPECT_FLOAT_EQ(light.GetSpotAngle(), 45.0f);
    
    light.SetSpotAngle(120.0f);
    EXPECT_FLOAT_EQ(light.GetSpotAngle(), 120.0f);
    
    // 角度应该限制在合理范围内
    light.SetSpotAngle(200.0f);
    EXPECT_FLOAT_EQ(light.GetSpotAngle(), 180.0f);
}
```

**Step 2: 运行测试验证失败**

运行: `cmake --build --preset engine-windows-x64-debug --target tests`
预期: FAIL - "Light2D.h: No such file or directory"

**Step 3: 编写最小实现**

创建 `src/engine/graphic/2d/Light2D.h`:

```cpp
#pragma once

#include "math/MathTypes.h"
#include <memory>

namespace Prisma {
namespace Graphic {

/**
 * @brief 2D 光源类
 * 支持点光源、方向光和聚光灯
 */
class Light2D {
public:
    enum class Type {
        Point,       // 点光源
        Directional, // 方向光
        Spot         // 聚光灯
    };

    Light2D(Type type = Type::Point);
    ~Light2D() = default;

    // ========== 类型 ==========
    
    void SetType(Type type) { m_type = type; }
    Type GetType() const { return m_type; }
    
    bool IsDirectional() const { return m_type == Type::Directional; }
    bool IsSpot() const { return m_type == Type::Spot; }
    bool IsPoint() const { return m_type == Type::Point; }

    // ========== 位置与方向 ==========
    
    void SetPosition(const Vector2& position) { m_position = position; }
    const Vector2& GetPosition() const { return m_position; }
    
    void SetDirection(const Vector2& direction) { m_direction = glm::normalize(direction); }
    const Vector2& GetDirection() const { return m_direction; }

    // ========== 颜色与强度 ==========
    
    void SetColor(const Vector3& color) { m_color = color; }
    const Vector3& GetColor() const { return m_color; }
    
    void SetIntensity(float intensity) { m_intensity = glm::max(0.0f, intensity); }
    float GetIntensity() const { return m_intensity; }

    // ========== 范围 ==========
    
    void SetRadius(float radius) { m_radius = glm::max(0.0f, radius); }
    float GetRadius() const { return m_radius; }

    // ========== 聚光灯 ==========
    
    void SetSpotAngle(float angleDegrees);
    float GetSpotAngle() const { return m_spotAngle; }

    // ========== 阴影 ==========
    
    void SetCastShadows(bool cast) { m_castShadows = cast; }
    bool IsCastShadows() const { return m_castShadows; }

    // ========== 更新 ==========
    
    void Update(Timestep ts);

private:
    Type m_type = Type::Point;
    
    Vector2 m_position = {0.0f, 0.0f};
    Vector2 m_direction = {1.0f, 0.0f};
    
    Vector3 m_color = {1.0f, 1.0f, 1.0f};
    float m_intensity = 1.0f;
    
    float m_radius = 100.0f;
    float m_spotAngle = 45.0f;
    
    bool m_castShadows = false;
};

} // namespace Graphic
} // namespace Prisma
```

创建 `src/engine/graphic/2d/Light2D.cpp`:

```cpp
#include "Light2D.h"
#include <algorithm>

namespace Prisma {
namespace Graphic {

Light2D::Light2D(Type type)
    : m_type(type)
{
}

void Light2D::SetSpotAngle(float angleDegrees) {
    m_spotAngle = glm::clamp(angleDegrees, 0.0f, 180.0f);
}

void Light2D::Update(Timestep /*ts*/) {
    // 未来可以添加动画、闪烁等效果
}

} // namespace Graphic
} // namespace Prisma
```

**Step 4: 运行测试验证通过**

运行: `cmake --build --preset engine-windows-x64-debug --target tests`
预期: PASS

**Step 5: 提交**

```bash
git add src/engine/graphic/2d/Light2D.h src/engine/graphic/2d/Light2D.cpp tests/graphic/test_light2d.cpp
git commit -m "feat(graphic): add Light2D class for 2D lighting"
```

---

#### Task 1.2: 创建 Lighting2DPass 渲染 Pass

**文件:**
- 创建: `src/engine/graphic/2d/Lighting2DPass.h`
- 创建: `src/engine/graphic/2d/Lighting2DPass.cpp`
- 修改: `src/engine/graphic/CMakeLists.txt`

**Step 1: 编写失败的测试**

创建测试文件 `tests/graphic/test_lighting2d_pass.cpp`:

```cpp
#include <gtest/gtest.h>
#include "graphic/2d/Lighting2DPass.h"
#include "graphic/2d/Light2D.h"

using namespace Prisma::Graphic;

TEST(Lighting2DPassTest, Initialization) {
    Lighting2DPass pass;
    
    pass.Initialize(800, 600);
    
    EXPECT_TRUE(pass.IsInitialized());
    EXPECT_EQ(pass.GetWidth(), 800);
    EXPECT_EQ(pass.GetHeight(), 600);
}

TEST(Lighting2DPassTest, AddRemoveLights) {
    Lighting2DPass pass;
    pass.Initialize(800, 600);
    
    auto light1 = std::make_shared<Light2D>();
    auto light2 = std::make_shared<Light2D>();
    
    pass.AddLight(light1);
    pass.AddLight(light2);
    
    EXPECT_EQ(pass.GetLightCount(), 2);
    
    pass.RemoveLight(light1);
    EXPECT_EQ(pass.GetLightCount(), 1);
    
    pass.ClearLights();
    EXPECT_EQ(pass.GetLightCount(), 0);
}

TEST(Lighting2DPassTest, AmbientLight) {
    Lighting2DPass pass;
    pass.Initialize(800, 600);
    
    Vector3 ambientColor = {0.2f, 0.2f, 0.3f};
    pass.SetAmbientLight(ambientColor, 0.5f);
    
    EXPECT_EQ(pass.GetAmbientColor(), ambientColor);
    EXPECT_FLOAT_EQ(pass.GetAmbientIntensity(), 0.5f);
}

TEST(Lighting2DPassTest, MaxLightsLimit) {
    Lighting2DPass pass;
    pass.Initialize(800, 600);
    
    // 添加超过最大数量的光源
    for (int i = 0; i < 20; ++i) {
        auto light = std::make_shared<Light2D>();
        pass.AddLight(light);
    }
    
    // 应该限制在最大数量
    EXPECT_LE(pass.GetLightCount(), Lighting2DPass::MAX_LIGHTS);
}
```

**Step 2: 运行测试验证失败**

运行: `cmake --build --preset engine-windows-x64-debug --target tests`
预期: FAIL - "Lighting2DPass.h: No such file or directory"

**Step 3: 编写最小实现**

创建 `src/engine/graphic/2d/Lighting2DPass.h`:

```cpp
#pragma once

#include "Light2D.h"
#include "interfaces/IRenderPass.h"
#include "math/MathTypes.h"
#include <vector>
#include <memory>

namespace Prisma {
namespace Graphic {

class ITexture;
class IRenderTarget;

/**
 * @brief 2D 光照渲染 Pass
 * 处理 2D 场景的光照计算
 */
class Lighting2DPass : public IRenderPass {
public:
    static constexpr uint32_t MAX_LIGHTS = 16;

    Lighting2DPass();
    ~Lighting2DPass() override;

    // ========== 初始化 ==========
    
    void Initialize(uint32_t width, uint32_t height);
    void Shutdown();
    bool IsInitialized() const { return m_initialized; }
    
    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }

    // ========== 光源管理 ==========
    
    void AddLight(std::shared_ptr<Light2D> light);
    void RemoveLight(std::shared_ptr<Light2D> light);
    void ClearLights();
    size_t GetLightCount() const { return m_lights.size(); }
    
    const std::vector<std::shared_ptr<Light2D>>& GetLights() const { return m_lights; }

    // ========== 环境光 ==========
    
    void SetAmbientLight(const Vector3& color, float intensity);
    const Vector3& GetAmbientColor() const { return m_ambientColor; }
    float GetAmbientIntensity() const { return m_ambientIntensity; }

    // ========== 渲染 ==========
    
    void Begin();
    void Render() override;
    void End();
    
    // 获取光照贴图
    std::shared_ptr<ITexture> GetLightMap() const { return m_lightMap; }

private:
    bool m_initialized = false;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    
    std::vector<std::shared_ptr<Light2D>> m_lights;
    
    Vector3 m_ambientColor = {0.1f, 0.1f, 0.1f};
    float m_ambientIntensity = 0.5f;
    
    std::shared_ptr<ITexture> m_lightMap;
    std::shared_ptr<IRenderTarget> m_lightTarget;
};

} // namespace Graphic
} // namespace Prisma
```

创建 `src/engine/graphic/2d/Lighting2DPass.cpp`:

```cpp
#include "Lighting2DPass.h"
#include "interfaces/ITexture.h"
#include "interfaces/IRenderTarget.h"

namespace Prisma {
namespace Graphic {

Lighting2DPass::Lighting2DPass() {
}

Lighting2DPass::~Lighting2DPass() {
    Shutdown();
}

void Lighting2DPass::Initialize(uint32_t width, uint32_t height) {
    if (m_initialized) {
        return;
    }
    
    m_width = width;
    m_height = height;
    
    // TODO: 创建光照贴图纹理和渲染目标
    // m_lightMap = Texture::Create(width, height, TextureFormat::RGBA16F);
    // m_lightTarget = RenderTarget::Create(m_lightMap);
    
    m_initialized = true;
}

void Lighting2DPass::Shutdown() {
    m_lights.clear();
    m_lightMap.reset();
    m_lightTarget.reset();
    m_initialized = false;
}

void Lighting2DPass::AddLight(std::shared_ptr<Light2D> light) {
    if (!light) {
        return;
    }
    
    if (m_lights.size() >= MAX_LIGHTS) {
        // 达到最大光源数量，忽略新光源
        return;
    }
    
    m_lights.push_back(light);
}

void Lighting2DPass::RemoveLight(std::shared_ptr<Light2D> light) {
    auto it = std::find(m_lights.begin(), m_lights.end(), light);
    if (it != m_lights.end()) {
        m_lights.erase(it);
    }
}

void Lighting2DPass::ClearLights() {
    m_lights.clear();
}

void Lighting2DPass::SetAmbientLight(const Vector3& color, float intensity) {
    m_ambientColor = color;
    m_ambientIntensity = glm::max(0.0f, intensity);
}

void Lighting2DPass::Begin() {
    if (!m_initialized || !m_lightTarget) {
        return;
    }
    
    // 绑定光照渲染目标
    // m_lightTarget->Bind();
    // 清除为环境光颜色
    // glClearColor(m_ambientColor.r * m_ambientIntensity, 
    //              m_ambientColor.g * m_ambientIntensity,
    //              m_ambientColor.b * m_ambientIntensity, 1.0f);
    // glClear(GL_COLOR_BUFFER_BIT);
}

void Lighting2DPass::Render() {
    if (!m_initialized) {
        return;
    }
    
    // 渲染每个光源
    for (const auto& light : m_lights) {
        if (light) {
            light->Update(Timestep(0.0f)); // 更新光源状态
            // TODO: 渲染光源贡献
        }
    }
}

void Lighting2DPass::End() {
    if (!m_initialized || !m_lightTarget) {
        return;
    }
    
    // 解绑渲染目标
    // m_lightTarget->Unbind();
}

} // namespace Graphic
} // namespace Prisma
```

**Step 4: 运行测试验证通过**

运行: `cmake --build --preset engine-windows-x64-debug --target tests`
预期: PASS

**Step 5: 提交**

```bash
git add src/engine/graphic/2d/Lighting2DPass.h src/engine/graphic/2d/Lighting2DPass.cpp tests/graphic/test_lighting2d_pass.cpp
git commit -m "feat(graphic): add Lighting2DPass for 2D lighting rendering"
```

---

### 任务 2: 实现 ParticleSystem 粒子系统

#### Task 2.1: 创建 Particle 粒子结构

**文件:**
- 创建: `src/engine/graphic/2d/Particle.h`
- 创建: `src/engine/graphic/2d/Particle.cpp`

**Step 1: 编写失败的测试**

创建测试文件 `tests/graphic/test_particle.cpp`:

```cpp
#include <gtest/gtest.h>
#include "graphic/2d/Particle.h"

using namespace Prisma::Graphic;

TEST(ParticleTest, DefaultConstruction) {
    Particle particle;
    
    EXPECT_EQ(particle.position, Vector2(0.0f, 0.0f));
    EXPECT_EQ(particle.velocity, Vector2(0.0f, 0.0f));
    EXPECT_FLOAT_EQ(particle.lifetime, 1.0f);
    EXPECT_FLOAT_EQ(particle.age, 0.0f);
    EXPECT_TRUE(particle.alive);
}

TEST(ParticleTest, UpdateAges) {
    Particle particle;
    particle.lifetime = 1.0f;
    
    particle.Update(Timestep(0.5f));
    EXPECT_FLOAT_EQ(particle.age, 0.5f);
    EXPECT_TRUE(particle.alive);
    
    particle.Update(Timestep(0.6f));
    EXPECT_FLOAT_EQ(particle.age, 1.1f);
    EXPECT_FALSE(particle.alive);
}

TEST(ParticleTest, ColorInterpolation) {
    Particle particle;
    particle.startColor = Color(1.0f, 0.0f, 0.0f, 1.0f);
    particle.endColor = Color(0.0f, 0.0f, 1.0f, 0.0f);
    particle.lifetime = 2.0f;
    
    particle.Update(Timestep(1.0f));
    Color midColor = particle.GetCurrentColor();
    
    // 中间点应该是红蓝混合
    EXPECT_NEAR(midColor.r, 0.5f, 0.01f);
    EXPECT_NEAR(midColor.b, 0.5f, 0.01f);
    EXPECT_NEAR(midColor.a, 0.5f, 0.01f);
}

TEST(ParticleTest, SizeInterpolation) {
    Particle particle;
    particle.startSize = 10.0f;
    particle.endSize = 2.0f;
    particle.lifetime = 2.0f;
    
    particle.Update(Timestep(1.0f));
    float midSize = particle.GetCurrentSize();
    
    EXPECT_NEAR(midSize, 6.0f, 0.01f);
}
```

**Step 2: 运行测试验证失败**

运行: `cmake --build --preset engine-windows-x64-debug --target tests`
预期: FAIL - "Particle.h: No such file or directory"

**Step 3: 编写最小实现**

创建 `src/engine/graphic/2d/Particle.h`:

```cpp
#pragma once

#include "math/MathTypes.h"
#include "interfaces/RenderTypes.h"
#include "core/Timestep.h"

namespace Prisma {
namespace Graphic {

/**
 * @brief 单个粒子
 */
struct Particle {
    // 位置与运动
    Vector2 position = {0.0f, 0.0f};
    Vector2 velocity = {0.0f, 0.0f};
    Vector2 acceleration = {0.0f, 0.0f};
    
    // 颜色
    Color startColor = {1.0f, 1.0f, 1.0f, 1.0f};
    Color endColor = {1.0f, 1.0f, 1.0f, 0.0f};
    
    // 大小
    float startSize = 1.0f;
    float endSize = 0.0f;
    
    // 旋转
    float rotation = 0.0f;
    float rotationSpeed = 0.0f;
    
    // 生命周期
    float lifetime = 1.0f;
    float age = 0.0f;
    
    // 状态
    bool alive = true;
    
    // 更新粒子状态
    void Update(Timestep ts);
    
    // 获取当前颜色（插值）
    Color GetCurrentColor() const;
    
    // 获取当前大小（插值）
    float GetCurrentSize() const;
    
    // 获取生命周期比例 (0-1)
    float GetLifeRatio() const;
};

} // namespace Graphic
} // namespace Prisma
```

创建 `src/engine/graphic/2d/Particle.cpp`:

```cpp
#include "Particle.h"

namespace Prisma {
namespace Graphic {

void Particle::Update(Timestep ts) {
    if (!alive) {
        return;
    }
    
    float dt = static_cast<float>(ts);
    
    // 更新年龄
    age += dt;
    
    // 检查是否死亡
    if (age >= lifetime) {
        alive = false;
        return;
    }
    
    // 更新位置
    velocity += acceleration * dt;
    position += velocity * dt;
    
    // 更新旋转
    rotation += rotationSpeed * dt;
}

Color Particle::GetCurrentColor() const {
    if (!alive || lifetime <= 0.0f) {
        return endColor;
    }
    
    float t = GetLifeRatio();
    return Color(
        startColor.r + (endColor.r - startColor.r) * t,
        startColor.g + (endColor.g - startColor.g) * t,
        startColor.b + (endColor.b - startColor.b) * t,
        startColor.a + (endColor.a - startColor.a) * t
    );
}

float Particle::GetCurrentSize() const {
    if (!alive || lifetime <= 0.0f) {
        return endSize;
    }
    
    float t = GetLifeRatio();
    return startSize + (endSize - startSize) * t;
}

float Particle::GetLifeRatio() const {
    if (lifetime <= 0.0f) {
        return 1.0f;
    }
    return glm::clamp(age / lifetime, 0.0f, 1.0f);
}

} // namespace Graphic
} // namespace Prisma
```

**Step 4: 运行测试验证通过**

运行: `cmake --build --preset engine-windows-x64-debug --target tests`
预期: PASS

**Step 5: 提交**

```bash
git add src/engine/graphic/2d/Particle.h src/engine/graphic/2d/Particle.cpp tests/graphic/test_particle.cpp
git commit -m "feat(graphic): add Particle struct for particle system"
```

---

#### Task 2.2: 创建 ParticleEmitter 发射器

**文件:**
- 创建: `src/engine/graphic/2d/ParticleEmitter.h`
- 创建: `src/engine/graphic/2d/ParticleEmitter.cpp`

**Step 1: 编写失败的测试**

创建测试文件 `tests/graphic/test_particle_emitter.cpp`:

```cpp
#include <gtest/gtest.h>
#include "graphic/2d/ParticleEmitter.h"

using namespace Prisma::Graphic;

TEST(ParticleEmitterTest, DefaultConstruction) {
    ParticleEmitter emitter;
    
    EXPECT_FLOAT_EQ(emitter.GetEmissionRate(), 10.0f);
    EXPECT_EQ(emitter.GetMaxParticles(), 1000);
    EXPECT_TRUE(emitter.IsEmitting());
}

TEST(ParticleEmitterTest, EmitParticles) {
    ParticleEmitter emitter;
    emitter.SetEmissionRate(100.0f); // 每秒100个
    emitter.SetMaxParticles(100);
    
    // 模拟1秒
    for (int i = 0; i < 60; ++i) {
        emitter.Update(Timestep(1.0f / 60.0f));
    }
    
    // 应该发射了约100个粒子
    EXPECT_GE(emitter.GetAliveParticleCount(), 90);
    EXPECT_LE(emitter.GetAliveParticleCount(), 100);
}

TEST(ParticleEmitterTest, MaxParticlesLimit) {
    ParticleEmitter emitter;
    emitter.SetEmissionRate(1000.0f);
    emitter.SetMaxParticles(50);
    
    // 模拟足够长的时间
    for (int i = alive; i < 600; ++i) {
        emitter.Update(Timestep(1.0f / 60.0f));
    }
    
    // 不应该超过最大数量
    EXPECT_LE(emitter.GetAliveParticleCount(), 50);
}

TEST(ParticleEmitterTest, ClearParticles) {
    ParticleEmitter emitter;
    emitter.SetEmissionRate(100.0f);
    
    for (int i = 0; i < 60; ++i) {
        emitter.Update(Timestep(1.0f / 60.0f));
    }
    
    EXPECT_GT(emitter.GetAliveParticleCount(), 0);
    
    emitter.Clear();
    EXPECT_EQ(emitter.GetAliveParticleCount(), 0);
}
```

**Step 2: 运行测试验证失败**

运行: `cmake --build --preset engine-windows-x64-debug --target tests`
预期: FAIL - "ParticleEmitter.h: No such file or directory"

**Step 3: 编写最小实现**

创建 `src/engine/graphic/2d/ParticleEmitter.h`:

```cpp
#pragma once

#include "Particle.h"
#include "core/Timestep.h"
#include <vector>
#include <random>

namespace Prisma {
namespace Graphic {

/**
 * @brief 粒子发射器
 * 控制粒子的生成和更新
 */
class ParticleEmitter {
public:
    ParticleEmitter();
    ~ParticleEmitter() = default;

    // ========== 位置 ==========
    
    void SetPosition(const Vector2& position) { m_position = position; }
    const Vector2& GetPosition() const { return m_position; }

    // ========== 发射参数 ==========
    
    void SetEmissionRate(float rate) { m_emissionRate = glm::max(0.0f, rate); }
    float GetEmissionRate() const { return m_emissionRate; }
    
    void SetMaxParticles(uint32_t max) { m_maxParticles = max; }
    uint32_t GetMaxParticles() const { return m_maxParticles; }
    
    void SetEmitting(bool emitting) { m_emitting = emitting; }
    bool IsEmitting() const { return m_emitting; }

    // ========== 粒子属性范围 ==========
    
    void SetVelocityRange(const Vector2& min, const Vector2& max) {
        m_velocityMin = min;
        m_velocityMax = max;
    }
    
    void SetLifetimeRange(float min, float max) {
        m_lifetimeMin = glm::max(0.0f, min);
        m_lifetimeMax = glm::max(m_lifetimeMin, max);
    }
    
    void SetSizeRange(float min, float max) {
        m_startSizeMin = glm::max(0.0f, min);
        m_startSizeMax = glm::max(m_startSizeMin, max);
    }
    
    void SetColorRange(const Color& startMin, const Color& startMax,
                       const Color& endMin, const Color& endMax) {
        m_startColorMin = startMin;
        m_startColorMax = startMax;
        m_endColorMin = endMin;
        m_endColorMax = endMax;
    }

    // ========== 更新与发射 ==========
    
    void Update(Timestep ts);
    void Emit(uint32_t count);

    // ========== 粒子访问 ==========
    
    const std::vector<Particle>& GetParticles() const { return m_particles; }
    size_t GetAliveParticleCount() const;
    
    void Clear();

private:
    Vector2 m_position = {0.0f, 0.0f};
    
    float m_emissionRate = 10.0f;
    float m_emissionTimer = 0.0f;
    uint32_t m_maxParticles = 1000;
    bool m_emitting = true;
    
    // 粒子属性范围
    Vector2 m_velocityMin = {-50.0f, -50.0f};
    Vector2 m_velocityMax = {50.0f, 50.0f};
    
    float m_lifetimeMin = 0.5f;
    float m_lifetimeMax = 2.0f;
    
    float m_startSizeMin = 1.0f;
    float m_startSizeMax = 5.0f;
    float m_endSizeMin = 0.0f;
    float m_endSizeMax = 0.0f;
    
    Color m_startColorMin = {1.0f, 1.0f, 1.0f, 1.0f};
    Color m_startColorMax = {1.0f, 1.0f, 1.0f, 1.0f};
    Color m_endColorMin = {1.0f, 1.0f, 1.0f, 0.0f};
    Color m_endColorMax = {1.0f, 1.0f, 1.0f, 0.0f};
    
    std::vector<Particle> m_particles;
    
    // 随机数生成
    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_dist;
    
    float RandomFloat(float min, float max);
    Vector2 RandomVector2(const Vector2& min, const Vector2& max);
    Color RandomColor(const Color& min, const Color& max);
};

} // namespace Graphic
} // namespace Prisma
```

创建 `src/engine/graphic/2d/ParticleEmitter.cpp`:

```cpp
#include "ParticleEmitter.h"
#include <algorithm>
#include <chrono>

namespace Prisma {
namespace Graphic {

ParticleEmitter::ParticleEmitter()
    : m_rng(static_cast<unsigned>(std::chrono::high_resolution_clock::now().time_since_epoch().count()))
    , m_dist(0.0f, 1.0f)
{
    m_particles.reserve(m_maxParticles);
}

void ParticleEmitter::Update(Timestep ts) {
    float dt = static_cast<float>(ts);
    
    // 更新现有粒子
    for (auto& particle : m_particles) {
        if (particle.alive) {
            particle.Update(ts);
        }
    }
    
    // 发射新粒子
    if (m_emitting && m_particles.size() < m_maxParticles) {
        m_emissionTimer += dt;
        
        float emitInterval = 1.0f / m_emissionRate;
        while (m_emissionTimer >= emitInterval && m_particles.size() < m_maxParticles) {
            Emit(1);
            m_emissionTimer -= emitInterval;
        }
    }
    
    // 移除死亡粒子（可选：使用对象池优化）
    m_particles.erase(
        std::remove_if(m_particles.begin(), m_particles.end(),
            [](const Particle& p) { return !p.alive; }),
        m_particles.end()
    );
}

void ParticleEmitter::Emit(uint32_t count) {
    for (uint32_t i = 0; i < count && m_particles.size() < m_maxParticles; ++i) {
        Particle particle;
        
        particle.position = m_position;
        particle.velocity = RandomVector2(m_velocityMin, m_velocityMax);
        particle.acceleration = {0.0f, 0.0f}; // 可以添加重力等
        
        particle.lifetime = RandomFloat(m_lifetimeMin, m_lifetimeMax);
        particle.age = 0.0f;
        
        particle.startSize = RandomFloat(m_startSizeMin, m_startSizeMax);
        particle.endSize = RandomFloat(m_endSizeMin, m_endSizeMax);
        
        particle.startColor = RandomColor(m_startColorMin, m_startColorMax);
        particle.endColor = RandomColor(m_endColorMin, m_endColorMax);
        
        particle.rotation = 0.0f;
        particle.rotationSpeed = RandomFloat(-180.0f, 180.0f);
        
        particle.alive = true;
        
        m_particles.push_back(particle);
    }
}

size_t ParticleEmitter::GetAliveParticleCount() const {
    return std::count_if(m_particles.begin(), m_particles.end(),
        [](const Particle& p) { return p.alive; });
}

void ParticleEmitter::Clear() {
    m_particles.clear();
}

float ParticleEmitter::RandomFloat(float min, float max) {
    return min + m_dist(m_rng) * (max - min);
}

Vector2 ParticleEmitter::RandomVector2(const Vector2& min, const Vector2& max) {
    return Vector2(
        RandomFloat(min.x, max.x),
        RandomFloat(min.y, max.y)
    );
}

Color ParticleEmitter::RandomColor(const Color& min, const Color& max) {
    return Color(
        RandomFloat(min.r, max.r),
        RandomFloat(min.g, max.g),
        RandomFloat(min.b, max.b),
        RandomFloat(min.a, max.a)
    );
}

} // namespace Graphic
} // namespace Prisma
```

**Step 4: 运行测试验证通过**

运行: `cmake --build --preset engine-windows-x64-debug --target tests`
预期: PASS

**Step 5: 提交**

```bash
git add src/engine/graphic/2d/ParticleEmitter.h src/engine/graphic/2d/ParticleEmitter.cpp tests/graphic/test_particle_emitter.cpp
git commit -m "feat(graphic): add ParticleEmitter for particle emission control"
```

---

#### Task 2.3: 创建 ParticleSystem 组件

**文件:**
- 创建: `src/engine/graphic/2d/ParticleSystem.h`
- 创建: `src/engine/graphic/2d/ParticleSystem.cpp`
- 修改: `src/engine/graphic/CMakeLists.txt`

**Step 1: 编写失败的测试**

创建测试文件 `tests/graphic/test_particle_system.cpp`:

```cpp
#include <gtest/gtest.h>
#include "graphic/2d/ParticleSystem.h"

using namespace Prisma::Graphic;

TEST(ParticleSystemTest, AddEmitter) {
    ParticleSystem system;
    
    auto emitter1 = std::make_shared<ParticleEmitter>();
    auto emitter2 = std::make_shared<ParticleEmitter>();
    
    system.AddEmitter(emitter1);
    system.AddEmitter(emitter2);
    
    EXPECT_EQ(system.GetEmitterCount(), 2);
}

TEST(ParticleSystemTest, PlayPauseStop) {
    ParticleSystem system;
    
    auto emitter = std::make_shared<ParticleEmitter>();
    system.AddEmitter(emitter);
    
    system.Play();
    EXPECT_TRUE(system.IsPlaying());
    
    system.Pause();
    EXPECT_FALSE(system.IsPlaying());
    
    system.Stop();
    EXPECT_FALSE(system.IsPlaying());
}

TEST(ParticleSystemTest, UpdatePropagates) {
    ParticleSystem system;
    
    auto emitter = std::make_shared<ParticleEmitter>();
    emitter->SetEmissionRate(100.0f);
    system.AddEmitter(emitter);
    
    system.Play();
    
    // 更新应该传播到发射器
    for (int i = 0; i < 60; ++i) {
        system.Update(Timestep(1.0f / 60.0f));
    }
    
    EXPECT_GT(emitter->GetAliveParticleCount(), 0);
}
```

**Step 2: 运行测试验证失败**

运行: `cmake --build --preset engine-windows-x64-debug --target tests`
预期: FAIL - "ParticleSystem.h: No such file or directory"

**Step 3: 编写最小实现**

创建 `src/engine/graphic/2d/ParticleSystem.h`:

```cpp
#pragma once

#include "ParticleEmitter.h"
#include "Component.h"
#include <vector>
#include <memory>

namespace Prisma {
namespace Graphic {

class ITexture;

/**
 * @brief 粒子系统组件
 * 管理多个粒子发射器
 */
class ParticleSystem : public Component {
public:
    ParticleSystem();
    ~ParticleSystem() override = default;

    // ========== 发射器管理 ==========
    
    void AddEmitter(std::shared_ptr<ParticleEmitter> emitter);
    void RemoveEmitter(std::shared_ptr<ParticleEmitter> emitter);
    void ClearEmitters();
    size_t GetEmitterCount() const { return m_emitters.size(); }
    
    const std::vector<std::shared_ptr<ParticleEmitter>>& GetEmitters() const {
        return m_emitters;
    }

    // ========== 控制 ==========
    
    void Play();
    void Pause();
    void Stop();
    bool IsPlaying() const { return m_playing; }

    // ========== 纹理 ==========
    
    void SetTexture(std::shared_ptr<ITexture> texture) { m_texture = texture; }
    std::shared_ptr<ITexture> GetTexture() const { return m_texture; }

    // ========== Component 接口 ==========
    
    void Initialize() override {}
    void Update(Timestep ts) override;
    void Shutdown() override;
    
    void Render(RenderCommandContext* context) override;

private:
    std::vector<std::shared_ptr<ParticleEmitter>> m_emitters;
    std::shared_ptr<ITexture> m_texture;
    bool m_playing = true;
};

} // namespace Graphic
} // namespace Prisma
```

创建 `src/engine/graphic/2d/ParticleSystem.cpp`:

```cpp
#include "ParticleSystem.h"
#include "Renderer2D.h"

namespace Prisma {
namespace Graphic {

ParticleSystem::ParticleSystem() {
}

void ParticleSystem::AddEmitter(std::shared_ptr<ParticleEmitter> emitter) {
    if (emitter) {
        m_emitters.push_back(emitter);
    }
}

void ParticleSystem::RemoveEmitter(std::shared_ptr<ParticleEmitter> emitter) {
    auto it = std::find(m_emitters.begin(), m_emitters.end(), emitter);
    if (it != m_emitters.end()) {
        m_emitters.erase(it);
    }
}

void ParticleSystem::ClearEmitters() {
    m_emitters.clear();
}

void ParticleSystem::Play() {
    m_playing = true;
}

void ParticleSystem::Pause() {
    m_playing = false;
}

void ParticleSystem::Stop() {
    m_playing = false;
    for (auto& emitter : m_emitters) {
        emitter->Clear();
    }
}

void ParticleSystem::Update(Timestep ts) {
    if (!m_playing) {
        return;
    }
    
    for (auto& emitter : m_emitters) {
        if (emitter) {
            emitter->Update(ts);
        }
    }
}

void ParticleSystem::Shutdown() {
    m_emitters.clear();
    m_texture.reset();
}

void ParticleSystem::Render(RenderCommandContext* /*context*/) {
    if (!m_playing) {
        return;
    }
    
    // 渲染所有发射器的粒子
    for (const auto& emitter : m_emitters) {
        if (!emitter) {
            continue;
        }
        
        const auto& particles = emitter->GetParticles();
        for (const auto& particle : particles) {
            if (!particle.alive) {
                continue;
            }
            
            Color color = particle.GetCurrentColor();
            float size = particle.GetCurrentSize();
            
            // 使用 Renderer2D 绘制粒子
            if (m_texture) {
                Renderer2D::DrawQuad(
                    particle.position,
                    Vector2(size, size),
                    m_texture,
                    color
                );
            } else {
                Renderer2D::DrawQuad(
                    particle.position,
                    Vector2(size, size),
                    color
                );
            }
        }
    }
}

} // namespace Graphic
} // namespace Prisma
```

**Step 4: 运行测试验证通过**

运行: `cmake --build --preset engine-windows-x64-debug --target tests`
预期: PASS

**Step 5: 提交**

```bash
git add src/engine/graphic/2d/ParticleSystem.h src/engine/graphic/2d/ParticleSystem.cpp tests/graphic/test_particle_system.cpp
git commit -m "feat(graphic): add ParticleSystem component for game objects"
```

---

### 任务 3: 集成到 PacMan 游戏

#### Task 3.1: 更新 PacMan 使用新光照系统

**文件:**
- 修改: `projects/PacManGame/src/game/PacMan.cpp`
- 修改: `projects/PacManGame/src/game/PacMan.h`
- 修改: `projects/PacManGame/src/game/GameController.cpp`

**Step 1: 添加光照相关代码**

在 `PacMan.h` 中添加光照成员:

```cpp
#include "graphic/2d/Light2D.h"

// 在 PacMan 类中添加
private:
    std::shared_ptr<Prisma::Graphic::Light2D> m_pacManLight;
```

在 `PacMan.cpp` 的 `Initialize` 方法中创建光源:

```cpp
void PacMan::Initialize(const glm::ivec2& spawnPosition, GameBoard* board) {
    // ... 现有代码 ...
    
    // 创建 PacMan 光源
    m_pacManLight = std::make_shared<Prisma::Graphic::Light2D>();
    m_pacManLight->SetType(Prisma::Graphic::Light2D::Type::Point);
    m_pacManLight->SetColor({1.0f, 1.0f, 0.0f}); // 黄色光
    m_pacManLight->SetIntensity(1.5f);
    m_pacManLight->SetRadius(150.0f);
}
```

在 `Update` 方法中更新光源位置:

```cpp
void PacMan::Update(Prisma::Timestep ts) {
    // ... 现有代码 ...
    
    // 更新光源位置
    if (m_pacManLight) {
        m_pacManLight->SetPosition(m_position);
    }
}
```

**Step 2: 提交**

```bash
git add projects/PacManGame/src/game/PacMan.h projects/PacManGame/src/game/PacMan.cpp
git commit -m "feat(pacman): integrate Light2D for PacMan character"
```

---

#### Task 3.2: 添加粒子效果

**文件:**
- 修改: `projects/PacManGame/src/game/GameController.cpp`
- 修改: `projects/PacManGame/src/game/GameController.h`

**Step 1: 添加吃豆粒子效果**

在 `GameController.h` 中添加粒子系统:

```cpp
#include "graphic/2d/ParticleSystem.h"

// 在 GameController 类中添加
private:
    std::unique_ptr<Prisma::Graphic::ParticleSystem> m_particleSystem;
    std::shared_ptr<Prisma::Graphic::ParticleEmitter> m_pelletEmitter;
```

在 `GameController.cpp` 的 `Initialize` 方法中创建粒子系统:

```cpp
void GameController::Initialize() {
    // ... 现有代码 ...
    
    // 创建粒子系统
    m_particleSystem = std::make_unique<Prisma::Graphic::ParticleSystem>();
    
    // 创建吃豆粒子发射器
    m_pelletEmitter = std::make_shared<Prisma::Graphic::ParticleEmitter>();
    m_pelletEmitter->SetEmissionRate(0.0f); // 手动触发
    m_pelletEmitter->SetMaxParticles(100);
    m_pelletEmitter->SetLifetimeRange(0.3f, 0.6f);
    m_pelletEmitter->SetSizeRange(2.0f, 4.0f);
    m_pelletEmitter->SetColorRange(
        Color(1.0f, 1.0f, 1.0f, 1.0f),
        Color(1.0f, 1.0f, 1.0f, 1.0f),
        Color(1.0f, 1.0f, 1.0f, 0.0f),
        Color(1.0f, 1.0f, 1.0f, 0.0f)
    );
    
    m_particleSystem->AddEmitter(m_pelletEmitter);
}
```

在 `CheckPelletCollision` 方法中触发粒子:

```cpp
void GameController::CheckPelletCollision() {
    glm::ivec2 gridPos = m_pacman.GetGridPosition();
    TileType tile = m_board.GetTile(gridPos.x, gridPos.y);

    if (tile == TileType::Pellet) {
        m_board.SetTile(gridPos.x, gridPos.y, TileType::Empty);
        AddScore(PELLET_SCORE);
        
        // 触发吃豆粒子效果
        if (m_pelletEmitter) {
            m_pelletEmitter->SetPosition(m_pacman.GetPosition());
            m_pelletEmitter->Emit(10);
        }
    } else if (tile == TileType::PowerPellet) {
        m_board.SetTile(gridPos.x, gridPos.y, TileType::Empty);
        AddScore(POWER_PELLET_SCORE);
        ActivatePowerMode();
        
        // 触发能量豆粒子效果
        if (m_pelletEmitter) {
            m_pelletEmitter->SetPosition(m_pacman.GetPosition());
            m_pelletEmitter->Emit(30);
        }
    }
}
```

**Step 2: 提交**

```bash
git add projects/PacManGame/src/game/GameController.h projects/PacManGame/src/game/GameController.cpp
git commit -m "feat(pacman): add particle effects for pellet collection"
```

---

## 总结

### 已实现功能

1. **Lighting2D 光照系统**
   - `Light2D` - 2D 光源类（点光源、方向光、聚光灯）
   - `Lighting2DPass` - 2D 光照渲染 Pass

2. **ParticleSystem 粒子系统**
   - `Particle` - 粒子结构
   - `ParticleEmitter` - 粒子发射器
   - `ParticleSystem` - 粒子系统组件

3. **PacMan 游戏集成**
   - PacMan 角色光源
   - 吃豆粒子效果

### 后续优化建议

1. **性能优化**
   - 实现粒子对象池
   - 添加视锥剔除
   - 优化批处理渲染

2. **功能增强**
   - 添加更多粒子效果（死亡、能量模式等）
   - 实现阴影投射
   - 添加光照贴图烘焙

3. **工具支持**
   - 创建粒子编辑器
   - 添加光照调试可视化

---

**计划完成时间：** 约 3-4 天

**依赖项：**
- 现有 Renderer2D 系统
- 现有 SpriteRenderer 组件
- 现有 Component 系统
