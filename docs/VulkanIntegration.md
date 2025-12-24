# Android 平台集成计划

> **状态**: 🔲 规划中
> **优先级**: 高
> **依赖**: [PrismaAndroid](https://github.com/Excurs1ons/PrismaAndroid) Vulkan 运行时

## 概述

PrismaEngine 的 Android 支持将参考 Unreal Engine 的目录组织方式，将 PrismaAndroid 项目逐步迁移到引擎主仓库。

## PrismaAndroid 现状

PrismaAndroid 项目已包含约 1300 行功能完整的 Vulkan 实现：

- ✅ `VulkanContext` - Instance/Device/SwapChain 管理 (~250 行)
- ✅ `RendererVulkan` - 完整渲染循环 (~1300 行)
- ✅ `ShaderVulkan` - SPIR-V 着色器加载
- ✅ `TextureAsset` - 纹理资源管理
- ✅ 屏幕旋转支持 (SwapChain 重建)

## 参考 UE 的目录组织

Unreal Engine 的 Android 代码组织方式：

```
Engine/
├── Build/Android/                    # Android 构建相关
│   └── Java/src/com/epicgames/ue4/   # Java 源码
│       └── GameActivity.java         # 主 Activity
└── Source/Runtime/Android/           # C++ 运行时
    ├── AndroidApplication.cpp        # 应用入口
    ├── AndroidJNI.cpp                # JNI 绑定
    └── ...
```

**UE 的关键设计**：
- Java 和 C++ 都在主仓库，通过目录分离
- 构建时 UEBuildAndroid 生成 gradle 项目
- 平台特定代码用 `#if PLATFORM_ANDROID` 宏隔离

## PrismaEngine 迁移方案

### 目标目录结构

```
PrismaEngine/
├── src/engine/
│   ├── graphic/                      # 跨平台渲染代码
│   │   ├── RenderBackend.h           # 渲染后端抽象
│   │   ├── vulkan/                   # Vulkan 通用实现
│   │   │   ├── VulkanBackend.h/cpp
│   │   │   ├── VulkanContext.h/cpp   # 从 PrismaAndroid 迁移
│   │   │   ├── VulkanRenderer.h/cpp  # 从 PrismaAndroid 迁移
│   │   │   ├── VulkanShader.h/cpp
│   │   │   └── VulkanTexture.h/cpp
│   │   └── d3d12/                    # DirectX 12 实现
│   │       └── ...
│   └── platform/                     # 平台抽象层
│       ├── android/                  # Android 平台代码
│       │   ├── AndroidWindow.h/cpp   # 窗口管理
│       │   ├── AndroidApplication.h/cpp
│       │   └── AndroidJNI.h/cpp      # JNI 绑定层
│       └── windows/
│           └── ...
├── projects/android/                 # Android 项目模板
│   ├── Game/                         # 游戏应用
│   │   └── src/main/
│   │       ├── java/                 # Java 源码
│   │       │   └── com/prisma/engine/
│   │       │       └── GameActivity.java
│   │       └── cpp/                  # JNI 入口
│   │           └── android_main.cpp
│   └── Engine/                       # 引擎库项目
│       └── src/main/cpp/
│           └── jni/
│               └── engine_jni.cpp
└── third_party/                      # 第三方库
    └── prisma_android/               # 作为 submodule 引用
        └── app/src/main/cpp/
            ├── VulkanContext.{h,cpp}
            └── RendererVulkan.{h,cpp}
```

### 迁移阶段

| 阶段 | 内容 | 状态 |
|------|------|------|
| **Phase 1** | 渲染抽象层设计 | 🔄 进行中 |
| **Phase 2** | VulkanContext 迁移到 `src/engine/graphic/vulkan/` | ⏳ 计划中 |
| **Phase 3** | RendererVulkan 迁移，适配抽象接口 | ⏳ 计划中 |
| **Phase 4** | Shader/Texture 资源系统迁移 | ⏳ 计划中 |
| **Phase 5** | 平台层 (JNI/Activity) 整合 | ⏳ 计划中 |
| **Phase 6** | 集成测试与优化 | ⏳ 计划中 |

### 代码迁移策略

| PrismaAndroid | PrismaEngine | 迁移方式 |
|---------------|--------------|----------|
| `VulkanContext` | `graphic/vulkan/VulkanContext` | 直接迁移，去掉 JNI 依赖 |
| `RendererVulkan` | `graphic/vulkan/VulkanRenderer` | 重构为适配抽象接口 |
| `ShaderVulkan` | `graphic/vulkan/VulkanShader` | 统一着色器接口 |
| `TextureAsset` | `graphic/vulkan/VulkanTexture` | 统一资源接口 |
| `android_main.cpp` | `platform/android/AndroidJNI.cpp` | 提取 JNI 绑定层 |
| `GameActivity` | `projects/android/Game/src/main/java/...` | 保留，作为项目模板 |
| `Scene/GameObject` | 已有 ECS | **不迁移**，使用引擎架构 |

### 关键设计点

#### 1. JNI 分离

```cpp
// platform/android/AndroidJNI.cpp
#if PLATFORM_ANDROID

#include "graphic/vulkan/VulkanRenderer.h"

extern "C" JNIEXPORT void JNICALL
Java_com_prisma_engine_GameActivity_nativeInit(
    JNIEnv* env,
    jobject thiz,
    jobject surface
) {
    // 初始化引擎
    Engine::Initialize();
}

#endif
```

#### 2. 平台宏隔离

```cpp
// graphic/RenderBackend.h

#if PLATFORM_WINDOWS
    #include "graphic/d3d12/D3D12Backend.h"
#elif PLATFORM_ANDROID
    #include "graphic/vulkan/VulkanBackend.h"
#endif
```

#### 3. PrismaAndroid 作为 Submodule

```bash
# 添加为 submodule
git submodule add https://github.com/Excurs1ons/PrismaAndroid.git third_party/prisma_android

# 迁移过程中直接引用源码
# 迁移完成后移除 submodule
```

## CMake 集成

### Android 交叉编译配置

```cmake
# CMakeLists.txt

if(ANDROID)
    # Android 平台特定配置
    find_package(Vulkan REQUIRED)

    # 引擎库
    add_library(PrismaEngine STATIC
        src/engine/graphic/vulkan/VulkanContext.cpp
        src/engine/graphic/vulkan/VulkanRenderer.cpp
        src/engine/platform/android/AndroidJNI.cpp
        # ...
    )

    target_link_libraries(PrismaEngine
        Vulkan::Vulkan
        android
        log
        EGL
    )
elseif(WIN32)
    # Windows 平台配置
    # ...
endif()
```

## 开发优先级

### 高优先级
1. **渲染抽象层设计** - 先定义 `RenderBackend` 接口
2. **Vulkan 核心迁移** - `VulkanContext` + `VulkanRenderer`

### 中优先级
1. **资源系统统一** - Shader/Texture 接口
2. **JNI 层封装** - 平台调用接口

### 低优先级
1. **构建系统完善** - Gradle 集成
2. **示例项目** - Android Demo

## 相关链接

- [PrismaAndroid Repository](https://github.com/Excurs1ons/PrismaAndroid)
- [Unreal Engine Directory Structure](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-directory-structure)
- [Vulkan Guide](https://vulkan-guide.com/)
- [Android NDK Guide](https://developer.android.com/ndk/guides)

---

*文档创建时间: 2025-12-25*
*最后更新: 2025-12-25*
