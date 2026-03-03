# PrismaEngine API 参考

## 版本 1.0.0

本文档提供 PrismaEngine 核心 API 的详细参考。

## 目录

- [核心系统](#核心系统)
- [渲染系统](#渲染系统)
- [音频系统](#音频系统)
- [输入系统](#输入系统)
- [资源管理](#资源管理)
- [物理系统](#物理系统)

---

## 核心系统

### Engine

引擎核心类，管理所有子系统。

```cpp
namespace PrismaEngine {

class Engine {
public:
    // 初始化引擎
    static bool Initialize(const EngineConfig& config);

    // 关闭引擎
    static void Shutdown();

    // 获取引擎实例
    static Engine* GetInstance();

    // 获取子系统
    InputManager* GetInputManager();
    RenderDevice* GetRenderDevice();
    AudioManager* GetAudioManager();
    AssetManager* GetAssetManager();

    // 运行引擎
    int Run();
};

} // namespace PrismaEngine
```

### Logger

日志系统。

```cpp
namespace PrismaEngine {

enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Fatal
};

class Logger {
public:
    // 初始化日志系统
    static void Init(const std::string& appName);

    // 记录日志
    template<typename... Args>
    static void Log(LogLevel level, const char* channel,
                    const char* format, Args&&... args);

    // 便捷宏
    #define LOG_TRACE(channel, ...) LOG(LogLevel::Trace, channel, __VA_ARGS__)
    #define LOG_DEBUG(channel, ...) LOG(LogLevel::Debug, channel, __VA_ARGS__)
    #define LOG_INFO(channel, ...) LOG(LogLevel::Info, channel, __VA_ARGS__)
    #define LOG_WARNING(channel, ...) LOG(LogLevel::Warning, channel, __VA_ARGS__)
    #define LOG_ERROR(channel, ...) LOG(LogLevel::Error, channel, __VA_ARGS__)
    #define LOG_FATAL(channel, ...) LOG(LogLevel::Fatal, channel, __VA_ARGS__)
};

} // namespace PrismaEngine
```

---

## 渲染系统

### IRenderDevice

渲染设备接口。

```cpp
namespace PrismaEngine {
namespace Graphic {

class IRenderDevice {
public:
    virtual ~IRenderDevice() = default;

    // 帧控制
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void Present() = 0;

    // 资源创建
    virtual std::shared_ptr<ITexture> CreateTexture(
        const TextureDesc& desc) = 0;
    virtual std::shared_ptr<IBuffer> CreateBuffer(
        const BufferDesc& desc) = 0;
    virtual std::shared_ptr<IShader> CreateShader(
        const ShaderDesc& desc) = 0;

    // 渲染命令
    virtual void SetRenderTarget(
        const std::shared_ptr<ITexture>& target) = 0;
    virtual void DrawMesh(
        const std::shared_ptr<IMesh>& mesh) = 0;

    // 清除
    virtual void Clear(const glm::vec4& color) = 0;
    virtual void ClearDepth(float depth) = 0;
};

} // namespace Graphic
} // namespace PrismaEngine
```

### Texture

纹理接口。

```cpp
namespace PrismaEngine {
namespace Graphic {

enum class TextureFormat {
    R8G8B8A8_UNORM,
    R8G8B8_UNORM,
    R16G16B16A16_FLOAT,
    R32G32B32A32_FLOAT,
    D24_UNORM_S8_UINT  // Depth-stencil
};

struct TextureDesc {
    uint32_t width;
    uint32_t height;
    TextureFormat format;
    const void* data;
    bool generateMips;
};

class ITexture {
public:
    virtual ~ITexture() = default;

    virtual void UpdateData(const void* data, uint32_t size) = 0;
    virtual void GenerateMips() = 0;
};

} // namespace Graphic
} // namespace PrismaEngine
```

---

## 音频系统

### IAudioDevice

音频设备接口。

```cpp
namespace PrismaEngine {
namespace Audio {

class IAudioDevice {
public:
    virtual ~IAudioDevice() = default;

    // 播放控制
    virtual void Play() = 0;
    virtual void Pause() = 0;
    virtual void Stop() = 0;

    // 音量控制
    virtual void SetMasterVolume(float volume) = 0;
    float GetMasterVolume() const = 0;

    // 音频源管理
    virtual AudioSource* CreateAudioSource() = 0;
    virtual void DestroyAudioSource(AudioSource* source) = 0;
};

} // namespace Audio
} // namespace PrismaEngine
```

---

## 输入系统

### InputManager

输入管理器。

```cpp
namespace PrismaEngine {
namespace Input {

enum class KeyCode {
    // 字母键
    A, B, C, ..., Z,

    // 数字键
    Num0, Num1, ..., Num9,

    // 功能键
    F1, F2, ..., F12,

    // 特殊键
    Space, Enter, Tab, Escape,
    Shift, Ctrl, Alt,

    // 方向键
    Up, Down, Left, Right
};

enum class MouseButton {
    Left,
    Right,
    Middle
};

class InputManager {
public:
    // 键盘状态
    virtual bool IsKeyDown(KeyCode key) const = 0;
    virtual bool IsKeyPressed(KeyCode key) const = 0;
    virtual bool IsKeyReleased(KeyCode key) const = 0;

    // 鼠标状态
    virtual bool IsMouseButtonDown(MouseButton button) const = 0;
    virtual bool IsMouseButtonPressed(MouseButton button) const = 0;
    virtual bool IsMouseButtonReleased(MouseButton button) const = 0;

    // 鼠标位置
    virtual void GetMousePosition(float* x, float* y) const = 0;
    virtual void GetMouseDelta(float* dx, float* dy) const = 0;

    // 鼠标滚轮
    virtual float GetScrollWheel() const = 0;

    // 文本输入
    virtual const std::string& GetTextInput() const = 0;
};

} // namespace Input
} // namespace PrismaEngine
```

---

## 资源管理

### AssetManager

资源管理器。

```cpp
namespace PrismaEngine {
namespace Core {

class AssetManager {
public:
    // 加载资源
    template<typename T>
    std::shared_ptr<T> LoadAsset(const std::string& path);

    // 异步加载
    template<typename T>
    void LoadAssetAsync(const std::string& path,
        std::function<void(std::shared_ptr<T>)> callback);

    // 卸载资源
    void UnloadAsset(const std::string& path);

    // 预加载
    void Preload(const std::string& manifestPath);

    // 内存管理
    size_t GetMemoryUsage() const;
    void UnloadUnusedAssets();
};

} // namespace Core
} // namespace PrismaEngine
```

---

## 物理系统

### CollisionSystem

碰撞检测系统。

```cpp
namespace PrismaEngine {
namespace Physics {

struct AABB {
    glm::vec3 min;
    glm::vec3 max;

    bool Contains(const glm::vec3& point) const;
    bool Intersects(const AABB& other) const;
};

struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
};

struct RaycastHit {
    bool hit;
    float distance;
    glm::vec3 point;
    glm::vec3 normal;
    void* userData;
};

class CollisionSystem {
public:
    // AABB 碰撞检测
    virtual bool CheckAABB(const AABB& a, const AABB& b) const = 0;

    // 射线检测
    virtual bool Raycast(const Ray& ray, RaycastHit& hit,
                        float maxDistance = FLT_MAX) const = 0;

    // 扫描检测
    virtual bool SweepAABB(const AABB& box, const glm::vec3& direction,
                         RaycastHit& hit) const = 0;

    // 碰撞响应
    virtual void ResolveCollisions() = 0;
};

} // namespace Physics
} // namespace PrismaEngine
```

---

## 更多信息

- [快速入门](QuickStart.md)
- [平台支持](PlatformSupport.md)
- [迁移指南](MigrationGuide.md)
- [示例项目](../samples/)
