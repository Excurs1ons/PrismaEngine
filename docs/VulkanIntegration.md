# Android Platform Integration / Android 平台集成

> **Status / 状态**: ✅ Implemented / 已实现
> **Priority / 优先级**: High / 高
> **Rendering API / 渲染 API**: Vulkan 1.1+

## Overview / 概述

Prisma Engine supports Android platform with Vulkan rendering backend. The Android runtime is implemented in `src/runtime/android/` with complete Vulkan support.

Prisma Engine 支持 Android 平台，使用 Vulkan 渲染后端。Android 运行时在 `src/runtime/android/` 中实现，具有完整的 Vulkan 支持。

## Architecture / 架构

### Directory Structure / 目录结构

```
PrismaEngine/
├── src/runtime/android/           # Android runtime implementation
│   ├── AndroidRuntime.cpp         # Entry point (android_main)
│   ├── VulkanContext.*            # Vulkan context management
│   ├── RendererVulkan.*           # Vulkan renderer
│   ├── ShaderVulkan.*             # SPIR-V shader loading
│   ├── TextureAsset.*             # Texture loading
│   ├── CubemapTextureAsset.*      # Cubemap loading
│   ├── SkyboxRenderer.*           # Skybox rendering
│   ├── renderer/                  # Renderer implementation
│   │   ├── API/                   # Vulkan API wrappers
│   │   ├── BackgroundPass.*       # Background rendering
│   │   ├── OpaquePass.*           # Opaque geometry
│   │   └── RenderPipeline.*       # Render pipeline
│   └── stb_impl.cpp              # STB library implementation
│
├── resources/common/shaders/      # Shared shader source
│   ├── hlsl/                     # HLSL (for DX12)
│   └── glsl/                     # GLSL (for Vulkan/OpenGL)
│       ├── clearcolor.vert/frag
│       ├── shader.vert/frag
│       └── skybox.vert/frag
│
├── resources/runtime/android/     # Android-specific resources
│   └── icons/                    # App icons
│
└── projects/android/PrismaAndroid/ # Android Studio project
    └── app/
        ├── src/main/
        │   ├── cpp/               # JNI glue code
        │   ├── java/              # MainActivity.java
        │   ├── assets/            # Runtime assets (copied during build)
        │   └── res/               # Android resources (icons, etc.)
        └── build.gradle.kts       # Gradle build config
```

## Key Components / 核心组件

### 1. AndroidRuntime / Android 运行时

Entry point for Android applications.

Android 应用的入口点。

```cpp
// src/runtime/android/AndroidRuntime.cpp

extern "C" void android_main(struct android_app* app) {
    // Initialize logging / 初始化日志
    // Create renderer / 创建渲染器
    // Enter game loop / 进入游戏循环
}
```

### 2. VulkanContext / Vulkan 上下文

Manages Vulkan instance, device, and swapchain.

管理 Vulkan 实例、设备和交换链。

Located in `src/runtime/android/VulkanContext.*`:

位置：`src/runtime/android/VulkanContext.*`：

```cpp
class VulkanContext {
public:
    // Initialize Vulkan / 初始化 Vulkan
    bool Initialize(android_app* app);

    // Manage swapchain / 管理交换链
    void CreateSwapchain();
    void RecreateSwapchain();  // For screen rotation / 用于屏幕旋转

    // Get Vulkan handles / 获取 Vulkan 句柄
    VkInstance GetInstance() const;
    VkDevice GetDevice() const;
    VkQueue GetGraphicsQueue() const;

private:
    VkInstance m_instance;
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;
    VkSwapchainKHR m_swapchain;
    // ... other Vulkan objects
};
```

### 3. RendererVulkan / Vulkan 渲染器

Complete Vulkan rendering implementation (~1456 lines).

完整的 Vulkan 渲染实现（约 1456 行）。

Located in `src/runtime/android/RendererVulkan.*`:

位置：`src/runtime/android/RendererVulkan.*`：

- **RenderPass** creation and management
- **GraphicsPipeline** creation
- **CommandBuffer** recording
- **Synchronization** (fences, semaphores)
- **Swapchain** presentation

### 4. Shader Loading / 着色器加载

```cpp
// SPIR-V shader loading / SPIR-V 着色器加载
class ShaderVulkan {
public:
    static std::vector<uint32_t> loadShader(
        AAssetManager* assetManager,
        const std::string& fileName
    );
};
```

**Usage / 用法**：
```cpp
auto vertShaderCode = ShaderVulkan::loadShader(
    assetManager, "shaders/skybox.vert.spv"
);
```

### 5. Texture Loading / 纹理加载

```cpp
// Texture loading via AAssetManager / 通过 AAssetManager 加载纹理
class TextureAsset {
public:
    static VkImage Load(
        AAssetManager* assetManager,
        const std::string& assetPath,
        VulkanContext* vulkanContext
    );
};
```

## Shader Compilation / 着色器编译

### Automatic Compilation / 自动编译

Android Gradle Plugin **automatically compiles** GLSL shaders to SPIR-V:

Android Gradle Plugin **自动编译** GLSL 着色器为 SPIR-V：

```
app/src/main/assets/shaders/
├── glsl/
│   ├── skybox.vert          # GLSL source / GLSL 源码
│   └── skybox.frag
│
   ↓ AGP自动编译 / AGP auto-compile ↓

build/intermediates/shader_assets/
└── shaders/
    ├── skybox.vert.spv      # SPIR-V bytecode / SPIR-V 字节码
    └── skybox.frag.spv
```

### How It Works / 工作原理

1. Place GLSL files in `app/src/main/assets/shaders/`
2. Android Gradle Plugin detects `.vert` and `.frag` files
3. Automatically calls `glslangValidator` during build
4. SPIR-V files are included in APK at `assets/shaders/`

### Accessing Shaders / 访问着色器

```cpp
// Load from assets / 从 assets 加载
auto vertShader = ShaderVulkan::loadShader(
    assetManager,
    "shaders/skybox.vert.spv"  // Path relative to assets/
);
```

## Asset Management / 资产管理

### Asset Paths / 资产路径

Assets in Android are accessed via `AAssetManager`:

Android 中的资产通过 `AAssetManager` 访问：

| Code Path / 代码路径 | Actual Location / 实际位置 |
|---------------------|-------------------------|
| `"shaders/skybox.vert.spv"` | `assets/shaders/skybox.vert.spv` |
| `"textures/android_robot.png"` | `assets/textures/android_robot.png` |

### Asset Copying / 资产复制

During build, Gradle copies resources to assets:

构建期间，Gradle 将资源复制到 assets：

```kotlin
// app/build.gradle.kts

tasks.register<Copy>("copyEngineRuntimeAssets") {
    // Copy common shaders / 复制通用着色器
    from("$engineRoot/resources/common/shaders/glsl") {
        into("shaders")
    }
    // Copy common textures / 复制通用纹理
    from("$engineRoot/resources/common/textures") {
        into("textures")
    }
    // Copy Android-specific resources / 复制 Android 特定资源
    from("$engineRoot/resources/runtime/android") {
        exclude("shaders")
        exclude("textures")
    }
    into("src/main/assets")
}

preBuild.dependsOn("copyEngineRuntimeAssets")
```

## Building / 构建

### Using Android Studio / 使用 Android Studio

1. Open `projects/android/PrismaAndroid` as a project
2. Click "Run" or "Debug"
3. APK is automatically built and installed

### Using Gradle / 使用 Gradle

```bash
cd projects/android/PrismaAndroid

# Build debug APK
./gradlew assembleDebug

# Build release APK
./gradlew assembleRelease

# Install to device
./gradlew installDebug
```

### Build Outputs / 构建输出

```
app/build/outputs/apk/
├── debug/app-debug.apk
└── release/app-release.apk
```

## Screen Rotation Support / 屏幕旋转支持

Android runtime properly handles screen rotation:

Android 运行时正确处理屏幕旋转：

```cpp
// Handle configuration changes / 处理配置变化
void RendererVulkan::onConfigChanged() {
    // Recreate swapchain / 重建交换链
    vulkanContext_.RecreateSwapchain();

    // Recreate render pass / 重建渲染通道
    // Recreate pipelines / 重建管线
}
```

## Debugging / 调试

### Logging / 日志

Android uses `AndroidOut.h` for logging:

Android 使用 `AndroidOut.h` 进行日志记录：

```cpp
#include "AndroidOut.h"

aout << "Message: " << value << std::endl;
```

### GPU Debugging / GPU 调试

- **RenderDoc**: Capture Vulkan frames
- **Android Studio GPU Inspector**: Real-time profiling
- **VK_LAYER_KHRONOS_validation**: Validation layer

### Common Issues / 常见问题

| Issue / 问题 | Solution / 解决方案 |
|-------------|-------------------|
| Shader not found / 着色器未找到 | Check path is relative to assets/ / 检查路径是否相对于 assets/ |
| SPIR-V compilation error / SPIR-V 编译错误 | Check GLSL syntax / 检查 GLSL 语法 |
| Swapchain creation failed / 交换链创建失败 | Check Vulkan support / 检查 Vulkan 支持 |
| Texture loading failed / 纹理加载失败 | Verify asset is in assets/ / 确认资产在 assets/ 中 |

## Platform-Specific Features / 平台特定功能

### Touch Input / 触摸输入

```cpp
// Handle touch events / 处理触摸事件
if (motionEvent->action == AMOTION_EVENT_ACTION_DOWN) {
    // Get touch coordinates / 获取触摸坐标
    float x = motionEvent->pointerCoords[0].getX();
    float y = motionEvent->pointerCoords[0].getY();
}
```

### Lifecycle Management / 生命周期管理

```cpp
// Handle lifecycle events / 处理生命周期事件
switch (cmd) {
    case APP_CMD_INIT_WINDOW:
        // Create renderer / 创建渲染器
        break;
    case APP_CMD_TERM_WINDOW:
        // Destroy renderer / 销毁渲染器
        break;
    case APP_CMD_WINDOW_REDRAW_NEEDED:
        // Handle screen rotation / 处理屏幕旋转
        break;
}
```

## Performance Considerations / 性能考虑

### Optimization Tips / 优化建议

1. **Shader compilation / 着色器编译**
   - Done at build time / 在构建时完成
   - No runtime compilation / 无运行时编译

2. **Texture loading / 纹理加载**
   - Use compressed formats / 使用压缩格式
   - Load asynchronously / 异步加载

3. **Synchronization / 同步**
   - Use fences for GPU-CPU sync / 使用 fence 进行 GPU-CPU 同步
   - Use semaphores for GPU-GPU sync / 使用 semaphore 进行 GPU-GPU 同步

## Integration with Engine / 与引擎集成

### Namespace Usage / 命名空间使用

```cpp
namespace PrismaEngine {
namespace Graphic {

// Vulkan backend for Android
class VulkanBackend {
    // Implementation...
};

} // namespace Graphic
} // namespace PrismaEngine
```

### Code Sharing / 代码共享

- **Common code** / 通用代码: `src/engine/graphic/`
- **Platform-specific** / 平台特定: `src/runtime/android/`

## Future Plans / 未来计划

- [ ] HDR rendering support / HDR 渲染支持
- [ ] VR rendering / VR 渲染
- [ ] Compute shaders / 计算着色器
- [ ] Multi-threaded command recording / 多线程命令记录

## Related Documentation / 相关文档

- [Directory Structure](DirectoryStructure.md) - File organization / 文件组织
- [Rendering System](RenderingSystem.md) - Rendering architecture / 渲染架构
- [Resource Management](ResourceManager.md) - Asset loading / 资产加载

## External Resources / 外部资源

- [Android NDK Guide](https://developer.android.com/ndk/guides/graphics/vulkan)
- [Vulkan Tutorial](https://vulkan-tutorial.com/)
- [Android Game Activity](https://developer.android.com/games/agdk/)
