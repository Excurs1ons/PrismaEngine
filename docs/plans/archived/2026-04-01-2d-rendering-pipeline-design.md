# 2D 渲染管线设计文档

## 概述

本文档描述了为 PrismaEngine 设计的完整 2D 渲染管线，专为 Pac-Man 游戏开发需求而设计。该系统将扩展现有的 Renderer2D，添加精灵动画、2D 光照和粒子系统支持。

## 设计目标

1. **高性能 2D 渲染**：支持大量精灵的高效渲染
2. **完整的动画系统**：支持关键帧动画、状态机动画
3. **2D 光照效果**：支持动态光源、阴影和光照贴图
4. **粒子系统**：支持各种粒子效果
5. **易于使用**：提供简洁的 API，与现有引擎架构无缝集成

## 系统架构

### 1. 核心组件

#### 1.1 SpriteAnimation 系统

**职责：**
- 管理精灵动画
- 支持关键帧动画
- 提供动画状态机
- 支持动画事件回调

**类设计：**
```cpp
class SpriteAnimation {
public:
    // 动画帧
    struct Frame {
        Vector4 spriteRect;  // x, y, width, height in texture
        float duration;      // 帧持续时间（秒）
    };

    // 动画状态
    enum class State {
        Playing,
        Paused,
        Stopped
    };

    // 构造函数
    SpriteAnimation();
    SpriteAnimation(const std::vector<Frame>& frames, float fps = 10.0f);
    
    // 动画控制
    void Play();
    void Pause();
    void Stop();
    void Reset();
    
    // 更新
    void Update(Timestep ts);
    
    // 获取当前帧
    const Frame& GetCurrentFrame() const;
    int GetCurrentFrameIndex() const;
    
    // 设置
    void SetLooping(bool loop) { m_looping = loop; }
    bool IsLooping() const { return m_looping; }
    
    void SetSpeed(float speed) { m_speed = speed; }
    float GetSpeed() const { return m_speed; }
    
    // 状态
    State GetState() const { return m_state; }
    bool IsFinished() const { return m_state == State::Stopped && !m_looping; }
    
    // 事件回调
    using FrameCallback = std::function<void(int frameIndex)>;
    using CompletionCallback = std::function<void()>;
    
    void SetFrameCallback(FrameCallback callback) { m_frameCallback = callback; }
    void SetCompletionCallback(CompletionCallback callback) { m_completionCallback = callback; }

private:
    std::vector<Frame> m_frames;
    float m_fps = 10.0f;
    float m_speed = 1.0f;
    bool m_looping = true;
    
    State m_state = State::Stopped;
    int m_currentFrame = 0;
    float m_frameTimer = 0.0f;
    
    FrameCallback m_frameCallback;
    CompletionCallback m_completionCallback;
};
```

#### 1.2 SpriteAnimator 组件

**职责：**
- 作为组件附加到游戏对象
- 管理多个动画
- 控制动画播放

**类设计：**
```cpp
class SpriteAnimator : public Component {
public:
    // 添加动画
    void AddAnimation(const std::string& name, std::shared_ptr<SpriteAnimation> animation);
    
    // 播放动画
    void PlayAnimation(const std::string& name);
    void PlayAnimation(const std::string& name, float speed);
    
    // 获取当前动画
    const std::string& GetCurrentAnimationName() const;
    std::shared_ptr<SpriteAnimation> GetCurrentAnimation() const;
    
    // 控制
    void Pause();
    void Resume();
    void Stop();
    
    // 更新
    virtual void Update(Timestep ts) override;
    
    // 获取当前帧信息（用于 SpriteRenderer）
    const Vector4& GetCurrentSpriteRect() const;

private:
    std::unordered_map<std::string, std::shared_ptr<SpriteAnimation>> m_animations;
    std::string m_currentAnimation;
    bool m_playing = false;
};
```

#### 1.3 Lighting2D 系统

**职责：**
- 管理 2D 光源
- 计算光照效果
- 支持阴影

**光源类型：**
```cpp
class Light2D {
public:
    enum class Type {
        Point,      // 点光源
        Directional, // 方向光
        Spot        // 聚光灯
    };
    
    // 属性
    Type type = Type::Point;
    Vector2 position = {0.0f, 0.0f};
    Vector3 color = {1.0f, 1.0f, 1.0f};
    float intensity = 1.0f;
    float radius = 100.0f;
    float angle = 360.0f;  // 聚光灯角度
    bool castShadows = false;
    
    // 更新
    void Update(Timestep ts);
    
    // 变换
    void SetPosition(const Vector2& pos) { position = pos; }
    void SetColor(const Vector3& col) { color = col; }
    void SetIntensity(float intens) { intensity = intens; }
    void SetRadius(float rad) { radius = rad; }
    
    // 渲染（内部使用）
    void Render(RenderCommandContext* context);
};
```

#### 1.4 Lighting2DPass

**职责：**
- 渲染 2D 光照效果
- 管理光照贴图
- 支持环境光

**类设计：**
```cpp
class Lighting2DPass : public RenderPass {
public:
    Lighting2DPass();
    
    // 光源管理
    void AddLight(std::shared_ptr<Light2D> light);
    void RemoveLight(std::shared_ptr<Light2D> light);
    void ClearLights();
    
    // 环境光
    void SetAmbientLight(const Vector3& color, float intensity);
    
    // 渲染设置
    void SetResolution(uint32_t width, uint32_t height);
    void SetShadowQuality(uint32_t quality);
    
    // 渲染
    virtual void Render(RenderContext* context) override;
    
    // 获取光照贴图
    std::shared_ptr<ITexture> GetLightMap() const;

private:
    std::vector<std::shared_ptr<Light2D>> m_lights;
    Vector3 m_ambientColor = {0.1f, 0.1f, 0.1f};
    float m_ambientIntensity = 0.5f;
    
    // 渲染资源
    std::shared_ptr<ITexture> m_lightMap;
    std::shared_ptr<IRenderTarget> m_lightTarget;
    
    // 着色器
    std::shared_ptr<IShader> m_lightingShader;
};
```

#### 1.5 Particle System

**职责：**
- 管理粒子效果
- 支持各种粒子属性
- 高效渲染大量粒子

**类设计：**
```cpp
class Particle {
public:
    Vector2 position = {0.0f, 0.0f};
    Vector2 velocity = {0.0f, 0.0f};
    Vector2 acceleration = {0.0f, 0.0f};
    
    Color startColor = {1.0f, 1.0f, 1.0f, 1.0f};
    Color endColor = {1.0f, 1.0f, 1.0f, 0.0f};
    
    float startSize = 1.0f;
    float endSize = 0.0f;
    
    float lifetime = 1.0f;
    float age = 0.0f;
    
    float rotation = 0.0f;
    float rotationSpeed = 0.0f;
    
    bool alive = true;
    
    void Update(Timestep ts);
    Color GetCurrentColor() const;
    float GetCurrentSize() const;
};

class ParticleEmitter {
public:
    // 发射器属性
    Vector2 position = {0.0f, 0.0f};
    Vector2 size = {10.0f, 10.0f};
    
    // 发射参数
    float emissionRate = 10.0f;  // 每秒发射粒子数
    float emissionTimer = 0.0f;
    
    // 粒子初始属性
    Vector2 velocityMin = {-50.0f, -50.0f};
    Vector2 velocityMax = {50.0f, 50.0f};
    
    float lifetimeMin = 0.5f;
    float lifetimeMax = 2.0f;
    
    float sizeMin = 1.0f;
    float sizeMax = 5.0f;
    
    Color startColorMin = {1.0f, 1.0f, 1.0f, 1.0f};
    Color startColorMax = {1.0f, 1.0f, 1.0f, 1.0f};
    
    Color endColorMin = {1.0f, 1.0f, 1.0f, 0.0f};
    Color endColorMax = {1.0f, 1.0f, 1.0f, 0.0f};
    
    // 发射控制
    bool emitting = true;
    int maxParticles = 1000;
    
    // 更新
    void Update(Timestep ts);
    
    // 发射粒子
    void Emit(int count);
    
    // 获取粒子列表
    const std::vector<Particle>& GetParticles() const;
    
    // 清除所有粒子
    void Clear();
};

class ParticleSystem : public Component {
public:
    ParticleSystem();
    
    // 发射器管理
    void AddEmitter(std::shared_ptr<ParticleEmitter> emitter);
    void RemoveEmitter(std::shared_ptr<ParticleEmitter> emitter);
    
    // 更新
    virtual void Update(Timestep ts) override;
    
    // 渲染
    virtual void Render(RenderCommandContext* context) override;
    
    // 控制
    void Play();
    void Pause();
    void Stop();
    
    // 纹理
    void SetTexture(std::shared_ptr<ITexture> texture);
    
private:
    std::vector<std::shared_ptr<ParticleEmitter>> m_emitters;
    std::shared_ptr<ITexture> m_texture;
    bool m_playing = true;
};
```

## 2. 渲染管线集成

### 2.1 渲染顺序

1. **背景层**：静态背景
2. **游戏对象层**：精灵、瓦片地图
3. **光照层**：2D 光照效果
4. **粒子层**：粒子效果
5. **UI 层**：用户界面

### 2.2 渲染流程

```cpp
class Pipeline2D {
public:
    void Render(RenderContext* context) {
        // 1. 开始场景
        Renderer2D::BeginScene(camera);
        
        // 2. 渲染游戏对象
        RenderGameObjects(context);
        
        // 3. 渲染光照
        m_lightingPass->Render(context);
        
        // 4. 渲染粒子
        RenderParticles(context);
        
        // 5. 结束场景
        Renderer2D::EndScene();
    }
    
private:
    std::unique_ptr<Lighting2DPass> m_lightingPass;
    std::vector<GameObject*> m_gameObjects;
};
```

## 3. 着色器设计

### 3.1 精灵着色器

**sprite2d.vert：**
```glsl
#version 460

layout(location = 0) in vec2 a_Position;
layout(location = 1) in vec2 a_TexCoord;
layout(location = 2) in vec4 a_Color;

layout(binding = 0) uniform CameraUBO {
    mat4 u_ViewProjection;
};

layout(location = 0) out vec2 v_TexCoord;
layout(location = 1) out vec4 v_Color;

void main() {
    gl_Position = u_ViewProjection * vec4(a_Position, 0.0, 1.0);
    v_TexCoord = a_TexCoord;
    v_Color = a_Color;
}
```

**sprite2d.frag：**
```glsl
#version 460

layout(location = 0) in vec2 v_TexCoord;
layout(location = 1) in vec4 v_Color;

layout(binding = 1) uniform sampler2D u_Texture;

layout(location = 0) out vec4 out_Color;

void main() {
    vec4 texColor = texture(u_Texture, v_TexCoord);
    out_Color = texColor * v_Color;
    
    if (out_Color.a < 0.01) {
        discard;
    }
}
```

### 3.2 光照着色器

**lighting2d.frag：**
```glsl
#version 460

layout(location = 0) in vec2 v_TexCoord;
layout(location = 1) in vec2 v_WorldPosition;

layout(binding = 0) uniform sampler2D u_AlbedoMap;
layout(binding = 1) uniform sampler2D u_NormalMap;

layout(binding = 2) uniform Light2DUBO {
    vec3 u_AmbientLight;
    int u_LightCount;
    Light2D u_Lights[16];
};

layout(location = 0) out vec4 out_Color;

struct Light2D {
    int type;
    vec2 position;
    vec3 color;
    float intensity;
    float radius;
    float angle;
    bool castShadows;
};

void main() {
    vec4 albedo = texture(u_AlbedoMap, v_TexCoord);
    vec3 normal = texture(u_NormalMap, v_TexCoord).xyz * 2.0 - 1.0;
    
    vec3 finalColor = u_AmbientLight;
    
    for (int i = 0; i < u_LightCount; i++) {
        Light2D light = u_Lights[i];
        
        // 计算光照
        vec3 lightDir = normalize(light.position - v_WorldPosition);
        float distance = length(v_WorldPosition - light.position);
        
        // 距离衰减
        float attenuation = 1.0 / (1.0 + 0.1 * distance + 0.01 * distance * distance);
        
        // 漫反射
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diff * light.color * light.intensity * attenuation;
        
        finalColor += diffuse;
    }
    
    out_Color = vec4(albedo.rgb * finalColor, albedo.a);
}
```

## 4. API 使用示例

### 4.1 精灵动画

```cpp
// 创建动画
auto walkAnimation = std::make_shared<SpriteAnimation>();
walkAnimation->AddFrame({{0.0f, 0.0f, 0.25f, 0.25f}, 0.1f});
walkAnimation->AddFrame({{0.25f, 0.0f, 0.25f, 0.25f}, 0.1f});
walkAnimation->AddFrame({{0.5f, 0.0f, 0.25f, 0.25f}, 0.1f});
walkAnimation->SetLooping(true);

// 添加到动画器
auto animator = gameObject->AddComponent<SpriteAnimator>();
animator->AddAnimation("walk", walkAnimation);
animator->PlayAnimation("walk");
```

### 4.2 2D 光照

```cpp
// 创建光源
auto light = std::make_shared<Light2D>();
light->type = Light2D::Type::Point;
light->position = {400.0f, 300.0f};
light->color = {1.0f, 0.8f, 0.6f};
light->intensity = 1.5f;
light->radius = 200.0f;

// 添加到光照系统
lightingPass->AddLight(light);
```

### 4.3 粒子效果

```cpp
// 创建粒子发射器
auto emitter = std::make_shared<ParticleEmitter>();
emitter->position = {400.0f, 300.0f};
emitter->velocityMin = {-100.0f, -100.0f};
emitter->velocityMax = {100.0f, 100.0f};
emitter->lifetimeMin = 0.5f;
emitter->lifetimeMax = 2.0f;
emitter->startColorMin = {1.0f, 0.5f, 0.0f, 1.0f};
emitter->startColorMax = {1.0f, 0.8f, 0.0f, 1.0f};
emitter->emissionRate = 50.0f;

// 添加到粒子系统
particleSystem->AddEmitter(emitter);
```

## 5. 性能优化

### 5.1 批处理优化

- **纹理批处理**：相同纹理的精灵在同一批次中渲染
- **顶点缓存**：重用顶点缓冲区
- **索引优化**：使用三角形条带减少索引数量

### 5.2 内存管理

- **对象池**：重用粒子对象
- **延迟加载**：纹理按需加载
- **内存对齐**：优化顶点数据结构

### 5.3 渲染优化

- **视锥剔除**：只渲染可见对象
- **LOD 系统**：远处对象使用简化渲染
- **多线程渲染**：并行处理非依赖操作

## 6. 集成指南

### 6.1 与现有系统集成

1. **Renderer2D**：扩展现有渲染器以支持新功能
2. **Component 系统**：新组件继承自 Component 基类
3. **资源管理**：使用现有纹理和着色器系统

### 6.2 文件结构

```
src/engine/graphic/
├── 2d/
│   ├── SpriteAnimation.h
│   ├── SpriteAnimation.cpp
│   ├── SpriteAnimator.h
│   ├── SpriteAnimator.cpp
│   ├── Lighting2D.h
│   ├── Lighting2D.cpp
│   ├── Lighting2DPass.h
│   ├── Lighting2DPass.cpp
│   ├── Particle.h
│   ├── Particle.cpp
│   ├── ParticleEmitter.h
│   ├── ParticleEmitter.cpp
│   ├── ParticleSystem.h
│   └── ParticleSystem.cpp
├── shaders/
│   ├── sprite2d.vert
│   ├── sprite2d.frag
│   ├── lighting2d.vert
│   └── lighting2d.frag
└── Pipeline2D.h
```

## 7. 测试计划

### 7.1 单元测试

- 精灵动画播放测试
- 粒子系统更新测试
- 光照计算测试

### 7.2 集成测试

- 渲染管线完整性测试
- 性能基准测试
- 内存泄漏测试

### 7.3 可视化测试

- Pac-Man 游戏场景测试
- 光照效果测试
- 粒子效果测试

## 8. 时间估算

| 组件 | 设计 | 实现 | 测试 | 总计 |
|------|------|------|------|------|
| SpriteAnimation | 1 天 | 2 天 | 1 天 | 4 天 |
| Lighting2D | 2 天 | 3 天 | 2 天 | 7 天 |
| ParticleSystem | 2 天 | 3 天 | 2 天 | 7 天 |
| 管线集成 | 1 天 | 2 天 | 1 天 | 4 天 |
| **总计** | **6 天** | **10 天** | **6 天** | **22 天** |

## 9. 风险与缓解

### 9.1 技术风险

- **性能问题**：大量粒子和光照可能影响性能
  - 缓解：实现 LOD 和视锥剔除
  
- **着色器兼容性**：不同平台着色器支持不同
  - 缓解：提供多个着色器版本

### 9.2 时间风险

- **复杂度高**：光照系统实现复杂
  - 缓解：分阶段实现，先实现基本功能

## 10. 总结

本设计提供了一个完整的 2D 渲染管线，包括精灵动画、2D 光照和粒子系统。该系统具有以下特点：

1. **模块化设计**：各组件独立，易于扩展
2. **高性能**：支持批处理和优化
3. **易用性**：提供简洁的 API
4. **可扩展性**：支持未来功能扩展

通过实现这个系统，PrismaEngine 将具备完整的 2D 游戏开发能力，为 Pac-Man 等 2D 游戏提供强大的渲染支持。