# PrismaEngine 缺失 API 文档
# 2D 吃豆人游戏开发需求分析

本文档记录了开发 2D 吃豆人游戏时需要的引擎功能以及当前引擎缺失的 API。

---

## 一、现有可用 API

### 1. 核心游戏对象系统
- `GameObject` - 游戏对象基类
- `Component` - 组件基类
- `Transform` - 变换组件（位置、旋转、缩放）

### 2. 渲染系统
- `RenderComponent` - 渲染组件
- `Material` - 材质系统
- `Renderer` - 高层渲染器接口
- `Mesh` / `IMesh` - 网格接口
- `TextureAsset` - 纹理加载
- `TextureAtlas` - 纹理图集
- `TilemapRenderer` - 瓦片地图渲染

### 3. 渲染管线
- `ForwardRenderPass` - 前向渲染 Pass
- `OpaquePass` - 不透明物体渲染
- `TransparentPass` - 透明物体渲染
- `UIPass` - UI 渲染 Pass

### 4. 光照系统
- `LightingPass::Light` - 光源结构
- `LightType::Point` - 点光源
- `LightType::Directional` - 方向光

### 5. 输入系统
- `EnhancedInputManager` - 增强输入管理器
- 键盘、鼠标、游戏手柄支持
- 输入映射系统

### 6. 物理系统
- `AABB` - 轴对齐包围盒
- `Ray` - 射线
- `RaycastHit` - 射线检测结果
- `CollisionSystem` - 碰撞检测

### 7. 音频系统
- `AudioAPI` - 音频 API
- `IAudioDevice` - 音频设备接口

---

## 二、缺失的 API（需补充）

### 1. 2D 精灵渲染器 [优先级：高]

**需求描述：**
- 支持 2D 精灵的渲染（吃豆人、幽灵）
- 支持精灵动画
- 支持精灵图集（Sprite Atlas）

**建议实现：**
```cpp
// src/engine/graphic/SpriteRenderer.h
class SpriteRenderer : public Component {
public:
    // 设置精灵纹理
    void SetSpriteTexture(std::shared_ptr<ITexture> texture);

    // 设置精灵图集区域
    void SetSpriteRect(float x, float y, float width, float height);

    // 设置颜色
    void SetColor(const Color& color);

    // 设置是否翻转
    void SetFlipX(bool flip);
    void SetFlipY(bool flip);

    // 渲染方法
    void Render(RenderCommandContext* context);
};
```

---

### 2. 2D 相机系统 [优先级：高]

**需求描述：**
- 正交投影相机
- 支持跟随目标
- 支持边界限制
- 视口缩放

**建议实现：**
```cpp
// src/engine/graphic/OrthographicCamera.h
class OrthographicCamera {
public:
    // 设置正交投影参数
    void SetOrthographic(float left, float right, float bottom, float top);

    // 设置跟随目标
    void SetFollowTarget(GameObject* target);
    void SetFollowOffset(const Vector2& offset);

    // 设置边界
    void SetBounds(const Vector2& minBounds, const Vector2& maxBounds);

    // 更新相机（每帧调用）
    void Update(Timestep ts);

    // 获取视图矩阵
    const Matrix4x4& GetViewMatrix() const;

    // 获取投影矩阵
    const Matrix4x4& GetProjectionMatrix() const;
};
```

---

### 3. 2D 光照 Pass [优先级：中]

**需求描述：**
- 专门的 2D 光照渲染 Pass
- 支持光照烘焙（可选）
- 支持动态光源
- 支持阴影投射

**建议实现：**
```cpp
// src/engine/graphic/pipelines/2d/Lighting2DPass.h
class Lighting2DPass : public LogicalPass {
public:
    // 2D 光源类型
    enum class LightType2D {
        Point,       // 点光源
        Directional, // 方向光（用于环境光）
        Spot         // 聚光灯
    };

    // 2D 光源结构
    struct Light2D {
        LightType2D type = LightType2D::Point;
        Vector2 position;      // 位置
        Vector3 color;         // RGB 颜色
        float intensity;       // 强度
        float radius;          // 影响半径
        float falloff;         // 衰减
        bool castShadows;     // 是否投射阴影
    };

    // 添加 2D 光源
    void AddLight2D(const Light2D& light);

    // 清除所有光源
    void ClearLights2D();

    // 设置环境光
    void SetAmbientLight(const Vector3& ambient);
};
```

---

### 4. 精灵动画系统 [优先级：中]

**需求描述：**
- 支持关键帧动画
- 支持动画混合
- 支持动画事件

**建议实现：**
```cpp
// src/engine/animation/SpriteAnimation.h
class SpriteAnimation {
public:
    // 动画帧
    struct Frame {
        float spriteRectX, spriteRectY;
        float spriteRectW, spriteRectH;
        float duration;
    };

    // 添加帧
    void AddFrame(const Frame& frame);

    // 播放动画
    void Play();
    void Pause();
    void Stop();

    // 设置循环
    void SetLooping(bool loop);

    // 更新动画
    void Update(Timestep ts);

    // 获取当前帧
    const Frame& GetCurrentFrame() const;

    // 是否完成
    bool IsFinished() const;
};

// 组件形式
class SpriteAnimationComponent : public Component {
public:
    // 添加动画
    void AddAnimation(const std::string& name, std::shared_ptr<SpriteAnimation> anim);

    // 播放动画
    void PlayAnimation(const std::string& name);
};
```

---

### 5. 2D 瓦片动画 [优先级：低]

**需求描述：**
- 瓦片动画已经存在（`TilemapRenderer` 已支持）
- 无需额外实现

---

### 6. 粒子系统 [优先级：低]

**需求描述：**
- 用于吃豆子时的特效
- 用于幽灵被吃时的特效

**建议实现：**
```cpp
// src/engine/particle/ParticleSystem.h
class Particle {
public:
    Vector2 position;
    Vector2 velocity;
    Color color;
    float lifetime;
    float size;
};

class ParticleSystem : public Component {
public:
    // 发射粒子
    void Emit(const Vector2& position, int count);

    // 设置发射器参数
    void SetEmitter(const ParticleEmitter& emitter);

    // 更新粒子
    void Update(Timestep ts);

    // 渲染粒子
    void Render(RenderCommandContext* context);
};
```

---

### 7. 路径寻找系统 [优先级：低]

**需求描述：**
- 用于幽灵 AI 的路径寻找
- A* 算法实现

**建议实现：**
```cpp
// src/engine/pathfinding/Pathfinding.h
class Pathfinding {
public:
    // A* 路径寻找
    static std::vector<Vector2> FindPath(
        const Vector2& start,
        const Vector2& goal,
        const std::vector<AABB>& obstacles
    );

    // 设置网格大小
    static void SetGridSize(float size);
};
```

---

## 三、所需着色器

### 1. 2D 精灵着色器 [需要创建]

```glsl
// sprite2d.vert
#version 460

layout(location = 0) in vec2 a_Position;
layout(location = 1) in vec2 a_TexCoord;

layout(binding = 0) uniform CameraUBO {
    mat4 u_ViewProjection;
};

layout(location = 0) out vec2 v_TexCoord;

void main() {
    gl_Position = u_ViewMatrix * vec4(a_Position, 0.0, 1.0);
    v_TexCoord = a_TexCoord;
}
```

```glsl
// sprite2d.frag
#version 460

layout(location = 0) in vec2 v_TexCoord;

layout(binding = 1) uniform sampler2D u_Texture;
layout(binding = 2) uniform SpriteUBO {
    vec4 u_Color;
    bool u_FlipX;
    bool u_FlipY;
};

layout(location = 0) out vec4 out_Color;

void main() {
    vec2 uv = v_TexCoord;
    if (u_FlipX) uv.x = 1.0 - uv.x;
    if (u_FlipY) uv.y = 1.0 - uv.y;

    vec4 texColor = texture(u_Texture, uv);
    out_Color = texColor * u_Color;

    if (out_Color.a < 0.01) {
        discard;
    }
}
```

### 2. 2D 光照着色器 [需要创建]

```glsl
// lighting2d.frag
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

layout(location) out vec4 out_Color;

struct Light2D {
    int type;
    vec2 position;
    vec3 color;
    float intensity;
    float radius;
    float falloff;
    bool castShadows;
};

void main() {
    vec4 albedo = texture(u_AlbedoMap, v_TexCoord);

    vec3 finalColor = u_AmbientLight;

    for (int i = 0; i < u_LightCount; i++) {
        Light2D light = u_Lights[i];
        float distance = length(v_WorldPosition - light.position);
        float attenuation = 1.0 / (1.0 + light.falloff * distance * distance);

        vec3 lightColor = light.color * light.intensity * attenuation;
        finalColor += lightColor;
    }

    out_Color = vec4(albedo.rgb * finalColor, albedo.a);
}
```

---

## 四、项目构建需求

### 1. CMake 配置需求

需要在 `projects/PacManGame/CMakeLists.txt` 中添加：

```cmake
# PacManGame 可执行文件
add_executable(PacManGame
    src/main.cpp
    src/PacManGame.cpp
    src/game/PacMan.cpp
    src/game/Ghost.cpp
    src/game/GameBoard.cpp
    src/game/GameController.cpp
    src/rendering/SpriteRenderer.cpp
    src/rendering/OrthographicCamera.cpp
)

# 链接引擎
target_link_libraries(PacManGame PRIVATE Engine)

# 复制资源文件
add_custom_command(TARGET PacManGame POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:PacManGame>/assets
)
```

### 2. 项目目录结构

```
projects/PacManGame/
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   ├── PacManGame.cpp
│   ├── game/
│   │   ├── PacMan.h/cpp
│   │   ├── Ghost.h/cpp
│   │   ├── GameBoard.h/cpp
│   │   ├── GameController.h/cpp
│   │   └── Pellet.h/cpp
│   └── rendering/
│       ├── SpriteRenderer.h/cpp
│       └── OrthographicCamera.h/cpp
└── assets/
    ├── sprites/
    │   ├── pacman.png
    │   └── ghosts.png
    ├── maps/
    │   └── pacman_level.tmx
    └── sounds/
        ├── waka.wav
        └── eat_ghost.wav
```

---

## 五、开发优先级

1. **高优先级（必须实现）：**
   - SpriteRenderer
   - OrthographicCamera
   - 项目构建系统

2. **中优先级（增强体验）：**
   - SpriteAnimation
   - Lighting2DPass

3. **低优先级（可选功能）：**
   - ParticleSystem
   - Pathfinding

---

## 六、总结

当前引擎已有完善的基础渲染和碰撞系统，但缺少专门面向 2D 游戏的高级组件。建议先实现以下核心功能：

1. `SpriteRenderer` - 2D 精灵渲染
2. `OrthographicCamera` - 2D 正交相机
3. 配置项目构建系统

完成这三项后，即可开始实现吃豆人游戏的核心逻辑。
