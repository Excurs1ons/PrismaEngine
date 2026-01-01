# Resource and Asset Management / 资源和资产管理

This document describes the asset and resource management system in Prisma Engine.

本文档描述 Prisma Engine 中的资产和资源管理系统。

## Overview / 概述

Prisma Engine uses an **asset-based resource management system** with the following features:

Prisma Engine 使用**基于资产的资源管理系统**，具有以下特点：

- **Unified interface** / 统一接口 - Single API for all asset types
- **Thread-safe loading** / 线程安全加载 - Asynchronous asset loading
- **Serialization support** / 序列化支持 - JSON and binary formats
- **Hot reload** / 热重载 - Runtime asset reloading (planned)
- **Fallback system** / 回退系统 - Default assets when loading fails

## Architecture / 架构

### Core Classes / 核心类

```
AssetBase (src/engine/core/AssetBase.h)
    │
    ├── Asset (src/engine/resource/Asset.h)
    │   ├── TextureAsset
    │   ├── MeshAsset
    │   └── [Custom assets]
    │
    └── AssetManager (src/engine/core/AssetManager.h)
        ├── Load<T>()
        ├── Unload()
        ├── Get()
        └── Reload()
```

### AssetManager / 资产管理器

The main entry point for asset operations.

资产操作的主要入口点。

```cpp
namespace PrismaEngine {

class AssetManager {
public:
    // Load an asset / 加载资产
    template<typename T>
    std::shared_ptr<T> Load(const std::filesystem::path& path);

    // Unload an asset / 卸载资产
    void Unload(const std::filesystem::path& path);

    // Get cached asset / 获取缓存的资产
    template<typename T>
    std::shared_ptr<T> Get(const std::filesystem::path& path);

    // Reload an asset / 重新加载资产
    void Reload(const std::filesystem::path& path);

    // Preload assets / 预加载资产
    void Preload(const std::vector<std::filesystem::path>& paths);
};

} // namespace PrismaEngine
```

### Asset Base Class / 资产基类

```cpp
namespace PrismaEngine {

class Asset {
public:
    virtual ~Asset() = default;

    // Serialize to JSON / 序列化为 JSON
    virtual nlohmann::json Serialize() const = 0;

    // Deserialize from JSON / 从 JSON 反序列化
    virtual void Deserialize(const nlohmann::json& data) = 0;

    // Get asset path / 获取资产路径
    std::filesystem::path GetPath() const { return m_path; }

    // Get asset state / 获取资产状态
    AssetState GetState() const { return m_state; }

protected:
    std::filesystem::path m_path;
    AssetState m_state;
};

} // namespace PrismaEngine
```

## Asset Types / 资产类型

### TextureAsset / 纹理资产

Located in `src/engine/resource/TextureAsset.h`.

```cpp
class TextureAsset : public Asset {
public:
    // Create from file / 从文件创建
    static std::shared_ptr<TextureAsset> Load(
        const std::filesystem::path& path
    );

    // Get texture handle / 获取纹理句柄
    Graphic::TextureHandle GetHandle() const;

    // Texture properties / 纹理属性
    uint32_t GetWidth() const;
    uint32_t GetHeight() const;
    Graphic::Format GetFormat() const;
};
```

### MeshAsset / 网格资产

Located in `src/engine/resource/MeshAsset.h`.

```cpp
class MeshAsset : public Asset {
public:
    // Load from file / 从文件加载
    static std::shared_ptr<MeshAsset> Load(
        const std::filesystem::path& path
    );

    // Get mesh data / 获取网格数据
    const std::vector<Vertex>& GetVertices() const;
    const std::vector<uint32_t>& GetIndices() const;

    // Get sub-meshes / 获取子网格
    const std::vector<SubMesh>& GetSubMeshes() const;
};
```

### Shader Assets / 着色器资产

Shaders are managed separately by the rendering system.

着色器由渲染系统单独管理。

See: `src/engine/graphic/Shader.h`

## Serialization / 序列化

### JSON Format / JSON 格式

Assets can be serialized to/from JSON format.

资产可以序列化为/从 JSON 格式。

```json
{
    "version": 1,
    "type": "TextureAsset",
    "path": "textures/player.png",
    "properties": {
        "filter": "linear",
        "wrap": "repeat",
        "generateMipmaps": true
    }
}
```

### Binary Format / 二进制格式

For faster loading, assets can be serialized in binary format.

为了更快的加载，资产可以序列化为二进制格式。

```cpp
// Binary archive / 二进制归档
namespace PrismaEngine {

class ArchiveBinary {
public:
    void Serialize(const Asset& asset);
    std::shared_ptr<Asset> Deserialize();
};

} // namespace PrismaEngine
```

## Resource Paths / 资源路径

### Path Resolution / 路径解析

The asset manager searches in the following locations:

资产管理器在以下位置搜索：

1. **Absolute paths** / 绝对路径 - Used as-is / 直接使用
2. **Relative to project** / 项目相对路径 - From project root
3. **Engine resources** / 引擎资源 - From `resources/` directory

### Setting Asset Paths / 设置资产路径

```cpp
// In your application initialization / 在应用初始化时
AssetManager::GetInstance().SetAssetBasePath("./Assets");

// Or use environment variable / 或使用环境变量
// PRISMA_ASSETS_PATH=/path/to/assets
```

## Asset Loading Workflow / 资产加载工作流

```
1. Request Asset
   │
   ▼
2. Check Cache ──→ Found? ──→ Return Cached Asset
   │ Not Found
   ▼
3. Load from Disk
   │
   ▼
4. Deserialize
   │
   ▼
5. Upload to GPU (if applicable)
   │
   ▼
6. Cache Asset
   │
   ▼
7. Return Asset
```

## Platform-Specific Assets / 平台特定资产

### Windows Assets / Windows 资产

```
resources/runtime/windows/
└── icons/
    ├── Runtime.ico
    └── small.ico
```

### Android Assets / Android 资产

```
projects/android/PrismaAndroid/app/src/main/assets/
├── shaders/                 # Compiled SPIR-V
│   ├── skybox.vert.spv
│   └── skybox.frag.spv
└── textures/                # Runtime textures
    └── android_robot.png
```

**Note**: Android assets are accessed via `AAssetManager`.

**注意**：Android 资产通过 `AAssetManager` 访问。

```cpp
// Android asset loading / Android 资产加载
AAssetManager* assetManager = app->activity->assetManager;
auto shaderCode = ShaderVulkan::loadShader(
    assetManager, "shaders/skybox.vert.spv"
);
```

## Fallback System / 回退系统

When an asset fails to load, a fallback asset is used.

当资产加载失败时，使用回退资产。

```cpp
namespace PrismaEngine {

class ResourceFallback {
public:
    // Get fallback texture / 获取回退纹理
    static std::shared_ptr<TextureAsset> GetFallbackTexture();

    // Get fallback mesh / 获取回退网格
    static std::shared_ptr<MeshAsset> GetFallbackMesh();

    // Get fallback shader / 获取回退着色器
    static std::shared_ptr<Shader> GetFallbackShader();
};

} // namespace PrismaEngine
```

## Common Shaders / 通用着色器

### HLSL Shaders (DirectX 12) / HLSL 着色器

Located in `resources/common/shaders/hlsl/`:

```
hlsl/
├── default.hlsl             # Default mesh shader
├── Skybox.hlsl              # Skybox shader
└── Text.hlsl                # Text rendering shader
```

### GLSL Shaders (Vulkan/OpenGL) / GLSL 着色器

Located in `resources/common/shaders/glsl/`:

```
glsl/
├── clearcolor.vert/frag      # Clear color pass
├── shader.vert/frag          # Standard mesh shader
└── skybox.vert/frag          # Skybox shader
```

**Compilation** / 编译:
- **Windows**: HLSL compiled by fxc/dxc at build time
- **Android**: GLSL compiled to SPIR-V by Android Gradle Plugin
- **Linux**: GLSL compiled to SPIR-V by glslangValidator

## Asset Hot Reload / 资产热重载

Planned feature for development workflow.

开发工作流程的规划功能。

```cpp
// Enable hot reload / 启用热重载
AssetManager::GetInstance().EnableHotReload(true);

// Check for changes (called in game loop) / 检查更改（在游戏循环中调用）
AssetManager::GetInstance().CheckForChanges();
```

## Best Practices / 最佳实践

### Loading Assets / 加载资产

```cpp
// ✅ Good: Use asset manager / 使用资产管理器
auto texture = AssetManager::GetInstance().Load<TextureAsset>(
    "textures/player.png"
);

// ❌ Bad: Direct file access / 直接文件访问
auto texture = LoadTextureDirect("textures/player.png");
```

### Asset Lifecycle / 资产生命周期

```cpp
// Load during initialization / 在初始化时加载
void Game::Initialize() {
    m_playerTexture = AssetManager::Load<TextureAsset>(
        "textures/player.png"
    );
}

// Assets are automatically cached / 资产自动缓存
// No need to manually manage lifetime / 无需手动管理生命周期
```

### Thread Safety / 线程安全

```cpp
// Safe: Load from background thread / 从后台线程安全加载
std::thread([&]() {
    auto texture = AssetManager::Load<TextureAsset>("heavy_texture.png");
}).detach();

// The asset manager handles synchronization / 资产管理器处理同步
```

## Related Documentation / 相关文档
- [Directory Structure](DirectoryStructure.md) - Asset locations / 资产位置
- [Asset Serialization](AssetSerialization.md) - Serialization details / 序列化详情
- [Rendering System](RenderingSystem.md) - Shader and texture loading / 着色器和纹理加载
