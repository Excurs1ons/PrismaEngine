# PrismaEngine 引擎开发教学手册
## 从零开始的游戏引擎构建之路

> **阅读前提**: 熟悉 C++23 语法，了解计算机图形学基础概念，有 CMake 使用经验。
> **配套代码**: https://github.com/Excurs1ons/PrismaEngine (dev 分支)

---

## 目录结构

```
Part 1: 基础层
├── 第1章  构建系统与项目结构
├── 第2章  平台抽象层
├── 第3章  数学库设计
├── 第4章  内存管理系统
├── 第5章  日志与调试系统

Part 2: 核心引擎层
├── 第6章  引擎核心架构
├── 第7章  事件系统
├── 第8章  层（Layer）系统
├── 第9章  资产管理
├── 第10章 作业系统与多线程
├── 第11章 性能分析系统

Part 3: 渲染管线
├── 第12章 渲染API抽象层
├── 第13章 Vulkan后端实现
├── 第14章 渲染资源管理
├── 第15章 渲染管线系统
├── 第16章 渲染图（Render Graph）
├── 第17章 2D渲染系统
├── 第18章 相机与变换系统

Part 4: 场景、实体与游戏系统
├── 第19章 场景图与节点系统
├── 第20章 组件系统
├── 第21章 SoA实体池与双缓冲
├── 第22章 输入系统
├── 第23章 物理与碰撞系统
├── 第24章 动画系统
├── 第25章 音频系统
├── 第26章 粒子系统
├── 第27章 AI与导航系统
├── 第28章 地形系统
├── 第29章 水体系统

Part 5: 扩展系统与编辑器
├── 第30章 网络系统
├── 第31章 本地化系统
├── 第32章 控制台与CVar系统
├── 第33章 脚本系统（CoreCLR & Mono）
├── 第34章 编辑器架构
├── 第35章 MCP协议与AI集成
├── 第36章 ImGui集成
├── 第37章 WebUI编辑器

Part 6: 进阶主题
├── 第38章 着色器系统与SPIR-V管线
├── 第39章 序列化系统
├── 第40章 打包与分发
├── 第41章 跨平台与Android
```

---

# Part 1: 基础层

## 第1章 构建系统与项目结构

### 1.1 设计理念

> **"使依赖管理对开发者透明，让 CMake 承载所有重担"**

游戏引擎是一个庞大而复杂的软件项目。PrismaEngine 面对的构建挑战包括：
- 跨平台（Windows/Linux/Android）编译器差异
- 20+ 第三方库依赖（Vulkan SDK, SDL3, ImGui, GLM, glTF 等）
- 多个构建目标（Engine 核心库、Launcher 可执行文件、Editor、SDK）
- 可选模块（脚本系统、编辑器、测试）
- 不同编译模式（Debug/Release/Dist）的不同优化策略

**核心设计决策**: 使用 **CMake FetchContent** 作为默认依赖管理方式，而不是 vcpkg 或 Conan。

**为什么？**
- **零安装成本**: 开发者只需要克隆代码库，CMake 配置阶段自动下载并构建所有依赖
- **版本锁定**: `cmake/DependencyVersions.cmake` 中精确锁定每个依赖的 Git Tag/Commit
- **可复现构建**: 任何人都能得到完全相同的第三方库版本
- **可选回退**: 设置 `-DPRISMA_USE_FETCHCONTENT=OFF` 可使用系统包管理器

### 1.2 项目结构解析

```
PrismaEngine/
├── src/                    # 全部源代码
│   ├── engine/            # 引擎核心库 (Engine 目标)
│   │   ├── app/           # Engine/Application 顶层入口
│   │   ├── core/          # 核心抽象 (ISubSystem, Layer, Event, ECS, Handle)
│   │   ├── graphic/       # 图形/渲染系统
│   │   │   ├── adapters/  # 图形API适配器 (vulkan/)
│   │   │   ├── interfaces/# 图形API抽象接口
│   │   │   └── pipelines/ # 渲染管线 (forward, deferred, clustered, pathtracing)
│   │   ├── platform/      # 平台抽象层
│   │   ├── math/          # 数学库 (基于GLM)
│   │   ├── memory/        # 内存管理器
│   │   ├── logger/        # 日志系统
│   │   ├── input/         # 输入系统
│   │   ├── physics/       # 物理/碰撞系统
│   │   ├── audio/         # 音频系统
│   │   ├── scene/         # 场景管理
│   │   ├── scripting/     # C#/Mono 脚本宿主
│   │   ├── resource/      # 资源加载器
│   │   ├── threading/     # 线程池与作业系统
│   │   ├── profiling/     # CPU/GPU 性能分析
│   │   ├── animation/     # 动画系统
│   │   ├── particles/     # 粒子系统
│   │   ├── terrain/       # 地形系统
│   │   ├── water/         # 水体系统
│   │   ├── ai/            # AI系统 (行为树, FSM)
│   │   ├── navigation/    # 导航/寻路系统
│   │   ├── network/       # 网络系统
│   │   ├── localization/  # 本地化
│   │   ├── console/       # 控制台
│   │   ├── serialization/ # 序列化
│   │   ├── transform/     # 变换组件
│   │   ├── ui/            # UI系统
│   │   ├── object/        # 反射元数据
│   │   ├── packing/       # 打包管线
│   │   ├── utils/         # 工具函数
│   │   ├── window/        # 窗口管理
│   │   └── interfaces/    # 引擎公开接口
│   ├── editor/            # 编辑器
│   ├── launcher/          # 启动器
│   └── tests/             # 测试
├── cmake/                 # CMake 模块
├── resources/             # 运行时资源 (着色器, 纹理, 字体)
├── scripts/               # 构建/部署脚本
├── sdk/                   # SDK (公共头文件 + CMake 配置)
└── projects/              # 示例项目
```

### 1.3 CMake 架构详解

PrismaEngine 的 CMake 设计遵循**模块化包含**模式：

**顶层 CMakeLists.txt** (182行):
```
1. 项目声明 (project PRISMA ...)
2. 编译选项 (CompilerOptions.cmake)
3. 平台配置 (PlatformConfig.cmake)
4. 输出目录 (OutputDirectories.cmake)
5. 设备选项 (DeviceOptions.cmake - 裁剪图形API)
6. 依赖管理 (FetchThirdPartyDeps.cmake)
7. 目标配置 (EngineTargets/LauncherTargets/EditorTargets)
8. 添加子目录 (src/engine, src/editor, src/launcher, projects)
9. 安装/打包 (InstallConfig, PackagingConfig)
```

**关键 CMake 模块**:

| 文件 | 职责 |
|------|------|
| `CompilerOptions.cmake` | 设置 C++23 标准，编译器警告级别，架构优化标志 |
| `PlatformConfig.cmake` | 检测操作系统，设置平台特定链接库 |
| `DeviceOptions.cmake` | 裁剪图形API（Vulkan/DX12/OpenGL），强制禁用不需要的后端 |
| `FetchThirdPartyDeps.cmake` | 通过 FetchContent 管理所有第三方依赖 |
| `EngineTargets.cmake` | 构建 Engine 核心库，链接所有子系统 |
| `LauncherTargets.cmake` | 构建启动器可执行文件 |
| `EditorTargets.cmake` | 构建编辑器可执行文件 |
| `ProjectTargets.cmake` | 注册所有可执行目标，处理资源复制 |
| `SDKConfig.cmake` | 构建 SDK 安装包 |
| `PackagingConfig.cmake` | CPack 打包配置 (NSIS/TGZ/DEB/RPM) |

### 1.4 第三方依赖管理

```cmake
# cmake/DependencyVersions.cmake - 所有依赖版本锁定
set(PRISMA_GLM_VERSION           "1.0.1")
set(PRISMA_IMGUI_VERSION         "v1.91.8-docking")
set(PRISMA_VKB_VERSION           "v1.3.296")
set(PRISMA_VMA_VERSION           "v3.2.1")
set(PRISMA_GLFW_VERSION          "3.4")
set(PRISMA_GLTF_VERSION          "v0.9.0")
# ... 20+ 依赖
```

**FetchContent 模式**:
```cmake
# 简化示例
FetchContent_Declare(
    glm
    GIT_REPOSITORY  https://github.com/g-truc/glm
    GIT_TAG         ${PRISMA_GLM_VERSION}
)
FetchContent_MakeAvailable(glm)
```

### 1.5 构建目标关系

```
Engine (静态库)
  ├── src/engine/ 下的所有子系统
  └── 链接所有第三方库

Launcher (可执行文件)
  ├── main.cpp
  └── 链接 Engine

Editor (可执行文件)
  ├── src/editor/ (ImGui, MCP, WebUI)
  ├── 链接 Engine
  └── 链接 ImGui

Sample Projects (可执行文件)
  ├── projects/Prisma2D/, PathTracing3D/, PrismaCraft/ 等
  └── 通过 SDK 或直接链接 Engine
```

### 1.6 CMake Presets

`CMakePresets.json` 定义了标准构建配置：

```bash
# Debug 构建
cmake --preset linux-x64-debug
cmake --build build/linux-x64-debug --parallel

# Release 构建
cmake --preset linux-x64-release
cmake --build build/linux-x64-release --parallel
```

### 1.7 实现步骤指南

**第1步 - 初始化项目结构**:
```cmake
cmake_minimum_required(VERSION 3.31)
project(PRISMA VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 23)
```

**第2步 - 配置编译器**:
- MSVC: `/W4`, `/utf-8`, `/permissive-`
- GCC/Clang: `-Wall -Wextra -Wpedantic`, `-fvisibility=hidden`

**第3步 - 平台检测与配置**:
- Windows: WinMain, DirectX 链接库, 宽字符支持
- Linux: X11/XCB 链接, Wayland 支持
- Android: NDK, GameActivity, Vulkan
- Termux: 特殊路径处理

**第4步 - 依赖管理**:
- 优先 FetchContent (默认)
- 可选 vcpkg (PRISMA_USE_FETCHCONTENT=OFF)

**第5步 - 构建目标定义**:
- 静态库 `Engine` → 所有引擎子系统编译为一个 .a/.lib
- 可执行文件 `Launcher` → 入口点 + Engine 链接
- 可执行文件 `Editor` → Editor 代码 + Engine 链接

**第6步 - 安装/打包**:
- SDK 头文件同步 (`scripts/sync-sdk-headers.sh`)
- CPack 配置 (NSIS/ZIP/TGZ/DEB/RPM)

---

## 第2章 平台抽象层

### 2.1 设计理念

> **"一次编写，三端运行"**

游戏引擎必须运行在 Windows、Linux 和 Android 上。平台抽象层的目标是：
1. 隔离所有平台特定代码到最小区域
2. 对外提供统一的 C++ API
3. 编译时选择平台实现（条件编译）

### 2.2 核心接口

`src/engine/platform/Platform.h`:

```cpp
namespace Prisma {

class Platform {
public:
    // 初始化平台层
    static int Initialize();
    static void Shutdown();
    
    // 获取平台信息
    static std::string GetPlatformName();     // "Windows", "Linux", "Android"
    static std::string GetOSVersion();
    static std::string GetCPUName();
    static uint32_t GetCPUThreadCount();      // 获取可用 CPU 核心数
    
    // 系统信息
    static uint64_t GetTotalRAM();
    static uint64_t GetAvailableRAM();
    
    // 文件系统
    static std::string GetExecutablePath();
    static std::string GetWorkingDirectory();
    
    // 时间
    static double GetTime();                  // 高精度时间戳 (秒)
    
    // 消息框
    static void ShowMessageBox(const std::string& title, 
                                const std::string& message);
    
    // 动态库加载
    static void* LoadDynamicLibrary(const std::string& path);
    static void* GetDynamicSymbol(void* handle, const std::string& name);
    static void  UnloadDynamicLibrary(void* handle);
};

// 平台实现 (Platform.cpp 通过条件编译选择)
#if defined(PRISMA_PLATFORM_WINDOWS)
    // Windows 实现
#elif defined(PRISMA_PLATFORM_LINUX)
    // Linux 实现
#elif defined(PRISMA_PLATFORM_ANDROID)
    // Android 实现
#endif

} // namespace Prisma
```

### 2.3 设计要点

**条件编译策略**:
- 所有平台代码在单个 `.cpp` 文件中通过 `#if defined()` 分节
- 避免为每个平台创建单独文件（保持简单）
- 平台检测宏在 `cmake/PlatformConfig.cmake` 中定义

**动态库加载** (`DynamicLoader`):
- Windows: `LoadLibraryA` / `GetProcAddress`
- Linux/Android: `dlopen` / `dlsym`
- 用于: CoreCLR 宿主加载 `hostfxr`, 引擎插件加载

### 2.4 实现步骤指南

```cpp
// 1. 定义平台检测宏
#if defined(_WIN32) || defined(_WIN64)
    #define PRISMA_PLATFORM_WINDOWS 1
#elif defined(__ANDROID__)
    #define PRISMA_PLATFORM_ANDROID 1
#elif defined(__linux__)
    #define PRISMA_PLATFORM_LINUX 1
#endif

// 2. 实现时间函数 (跨平台高精度计时)
#if defined(PRISMA_PLATFORM_WINDOWS)
    double Platform::GetTime() {
        LARGE_INTEGER freq, counter;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&counter);
        return static_cast<double>(counter.QuadPart) / freq.QuadPart;
    }
#else
    #include <chrono>
    double Platform::GetTime() {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = now.time_since_epoch();
        return std::chrono::duration<double>(duration).count();
    }
#endif

// 3. 实现 CPU 核心数检测
uint32_t Platform::GetCPUThreadCount() {
    // C++11 标准方法
    return std::thread::hardware_concurrency();
}
```

---

## 第3章 数学库设计

### 3.1 设计理念

> **"不重复造数学轮子，但提供一致的类型体系"**

PrismaEngine 选择 **GLM (OpenGL Mathematics)** 作为底层数学库，因为它：
- 与 GLSL 着色语言语法一致
- 头文件库，零运行时开销
- 成熟的 SIMD 优化
- 活跃的社区支持

### 3.2 类型体系

`src/engine/math/MathTypes.h` 定义了统一类型别名：

```cpp
namespace Prisma {

// === 向量类型 ===
using Vector2   = glm::vec2;     // 2D 向量 (float)
using Vector3   = glm::vec3;     // 3D 向量 (float)
using Vector4   = glm::vec4;     // 4D 向量 (float)
using IVector2  = glm::ivec2;     // 2D 整型向量
using IVector3  = glm::ivec3;     // 3D 整型向量
using UVector2  = glm::uvec2;     // 2D 无符号整型向量

// === 矩阵类型 ===
using Matrix4x4 = glm::mat4;      // 4x4 变换矩阵
using Matrix3x3 = glm::mat3;      // 3x3 矩阵

// === 四元数 ===
using Quaternion = glm::quat;     // 旋转四元数

// === 颜色 ===
using Color     = Vector4;        // RGBA 颜色

// === 平面 ===
struct Plane {
    Vector3 normal;
    float distance;
};

// === 别名 ===
namespace PrismaMath = glm;       // 兼容旧代码

// === 数学常量 ===
constexpr float PI        = std::numbers::pi_v<float>;
constexpr float DEG_TO_RAD = PI / 180.0f;
constexpr float RAD_TO_DEG = 180.0f / PI;
constexpr float EPSILON    = 1e-6f;

// === 工具函数 ===
namespace Math {
    Matrix4x4 Translation(const Vector3& t);
    Matrix4x4 RotationX(float angle);
    Matrix4x4 RotationY(float angle);
    Matrix4x4 RotationZ(float angle);
    Matrix4x4 Scale(const Vector3& s);
    Matrix4x4 Perspective(float fov, float aspect, float near, float far);
    Matrix4x4 Orthographic(float left, float right, float bottom, float top);
    Matrix4x4 LookAt(const Vector3& eye, const Vector3& center, const Vector3& up);
}

} // namespace Prisma
```

### 3.3 设计要点

**为什么不封装 GLM？**
许多引擎选择封装数学库以实现 API 切换自由。PrismaEngine 选择**直接暴露 GLM 类型**：
- 减少抽象层的内存/性能开销
- 简化代码（没有 `Math::Vector3::Cross(a, b)`，直接用 `glm::cross(a, b)`）
- GLM 本身就是"标准"——熟悉 GLSL 的开发者无需学习新的数学 API
- 即使未来切换底层库，类型别名机制可以平滑迁移

**附加数学工具** (`src/engine/math/MatrixUtils.h`):
- Frustum 矩阵提取
- 变换分解（平移/旋转/缩放提取）
- 投影矩阵操作

**哈希函数** (`src/engine/math/MurmurHash3.h`):
- 用于资源路径的快速哈希
- 用于着色器/管线的缓存键

### 3.4 实现步骤指南

```cpp
// 第1步: 引入 GLM 头文件
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

// 第2步: 定义类型别名 (不要用 typedef，使用 using)
using Vector3 = glm::vec3;

// 第3步: 添加数学常量
constexpr float PI = 3.14159265358979323846f;

// 第4步: 实现辅助函数
inline Matrix4x4 Math::LookAt(const Vector3& eye, const Vector3& center, const Vector3& up) {
    return glm::lookAt(eye, center, up);
}

// 第5步: 添加额外的自定义类型 (非 GLM 提供的)
struct Plane { /* ... */ };
struct Frustum { Plane planes[6]; /* left, right, top, bottom, near, far */ };
```

---

## 第4章 内存管理系统

### 4.1 设计理念

> **"游戏引擎中，内存布局决定性能天花板"**

游戏引擎与传统应用的关键区别：
1. **每帧分配**: 游戏每帧创建和销毁大量临时对象
2. **缓存友好性**: 频繁遍历的组件数据应连续存储（SoA 模式）
3. **内存碎片**: 长期运行不能频繁触发 `malloc/free`
4. **确定性**: 不希望 GC 或 `new` 在渲染关键路径上触发延迟

### 4.2 内存系统架构

`src/engine/memory/` 包含：

```
memory/
├── MemorySystem.h      # ISubSystem 适配层
├── MemoryManager.h     # 全局内存管理器
├── MemoryAllocator.h   # 分配器基类
├── StackAllocator.h    # 栈分配器 (LIFO)
├── PoolAllocator.h     # 对象池分配器
├── FrameAllocator.h    # 帧分配器 (每帧重置)
└── MemorySystem.cpp    # 实现
```

### 4.3 核心分配器类型

**栈分配器 (Stack Allocator)**:
```cpp
class StackAllocator {
    // LIFO (后进先出) 分配模式
    // 适用于: 每帧的临时数据、场景加载时的批量分配
    // 特点: O(1) 分配/释放，零碎片
    
    void* Allocate(size_t size, size_t alignment = alignof(max_align_t));
    void Deallocate(void* ptr);              // 只能释放栈顶
    void Reset();                             // 重置整个栈
};
```

**对象池分配器 (Pool Allocator)**:
```cpp
class PoolAllocator {
    // 固定大小块的预分配池
    // 适用于: 实体、组件、渲染命令
    // 特点: O(1) 分配/释放，无碎片
    
    void* Allocate();
    void Deallocate(void* ptr);
};
```

**帧分配器 (Frame Allocator)**:
```cpp
class FrameAllocator {
    // 每帧开始重置的双缓冲线性分配器
    // 适用于: 每帧的渲染命令、变换数据
    // 特点: 无需手动释放，帧尾自动回收
    
    void* Allocate(size_t size);
    void Reset();  // 在 EndFrame() 时调用
};
```

### 4.4 MemoryManager 全局管理器

```cpp
class MemoryManager {
public:
    static MemoryManager& Get();
    
    // 初始化所有分配器
    void Initialize();
    void Shutdown();
    
    // 每帧事件
    void OnFrameBegin();
    void OnFrameEnd();
    
    // 获取分配器
    StackAllocator& GetFrameAllocator();
    PoolAllocator& GetEntityAllocator();
    
    // 统计
    AllocationStats GetAllocationStats() const;
};
```

### 4.5 设计要点

**双缓冲帧分配器**: 帧 N 使用缓冲区 A，帧 N+1 使用缓冲区 B，帧 N+2 复用 A。
这样前一帧的指针在整帧期间都有效，不会被下一帧的分配覆盖。

**内存对齐**: 所有分配器强制 `alignof(max_align_t)`（通常 16 字节），确保 SIMD 数据类型安全。

### 4.6 实现步骤指南

```cpp
// 第1步: 定义分配器基类
class IAllocator {
public:
    virtual void* Allocate(size_t size, size_t alignment = 16) = 0;
    virtual void Deallocate(void* ptr) = 0;
    virtual void Reset() {}
    virtual ~IAllocator() = default;
};

// 第2步: 实现栈分配器
class StackAllocator : public IAllocator {
    char* m_buffer;
    size_t m_capacity;
    size_t m_offset;
    
public:
    StackAllocator(size_t capacity) 
        : m_buffer(new char[capacity]), m_capacity(capacity), m_offset(0) {}
    
    ~StackAllocator() { delete[] m_buffer; }
    
    void* Allocate(size_t size, size_t alignment) override {
        // 对齐当前偏移
        size_t alignedOffset = (m_offset + alignment - 1) & ~(alignment - 1);
        if (alignedOffset + size > m_capacity) return nullptr;
        m_offset = alignedOffset + size;
        return m_buffer + alignedOffset;
    }
    
    void Deallocate(void* ptr) override {
        // 栈分配器不支持任意顺序释放
        // 只有 LIFO 顺序可行
    }
    
    void Reset() override { m_offset = 0; }
};

// 第3步: MemorySystem 包装为 ISubSystem
class MemorySystem : public ISubSystem {
    MemoryManager& m_manager;
public:
    int Initialize() override { /* ... */ return 0; }
    void Shutdown() override  { /* ... */ }
    void Update(Timestep ts) override { /* 每帧统计 */ }
};
```

---

## 第5章 日志与调试系统

### 5.1 设计理念

> **"日志是引擎的神经系统——它必须无时无刻都在工作"**

日志系统的设计目标：
1. **线程安全**: 任何线程任何时间都能安全调用
2. **零成本关闭**: Release 构建中日志宏完全消失
3. **结构化**: 日志条目包含时间戳、线程ID、文件/行号、严重级别
4. **可扩展**: 支持多个 Sink（控制台、文件、远程调试器）

### 5.2 Logger 架构

`src/engine/logger/`:

```cpp
namespace Prisma {

// 日志级别
enum class LogLevel {
    Trace = 0,
    Debug,
    Info,
    Warn,
    Error,
    Critical,
    Off
};

// 日志条目
struct LogEntry {
    LogLevel level;
    std::string message;
    std::string category;
    std::string file;
    int line;
    std::string function;
    std::chrono::system_clock::time_point timestamp;
    uint32_t threadId;
};

// 日志 Sink 接口
class ILogSink {
public:
    virtual ~ILogSink() = default;
    virtual void Write(const LogEntry& entry) = 0;
    virtual void Flush() = 0;
};

// 控制台 Sink (输出到 stdout/stderr, 带颜色)
class ConsoleSink : public ILogSink { /* ... */ };

// 文件 Sink (输出到日志文件，支持轮转)
class FileSink : public ILogSink { /* ... */ };

// 平台日志接口 (用于 Android logcat)
class IPlatformLogger {
public:
    virtual void Log(LogLevel level, const std::string& message) = 0;
};

// 核心 Logger
class Logger {
public:
    static Logger& Get();
    
    void AddSink(std::unique_ptr<ILogSink> sink);
    void SetMinLevel(LogLevel level);
    
    template<typename... Args>
    void Log(LogLevel level, const std::string& category, 
             const std::string& format, Args&&... args);
    
    // 便捷宏
    // LOG_TRACE("Category", "message {}", arg);
    // LOG_INFO("Category", "message {}", arg);
    // LOG_WARN("Category", "message {}", arg);
    // LOG_ERROR("Category", "message {}", arg);
    // LOG_CRITICAL("Category", "message {}", arg);
    
private:
    std::vector<std::unique_ptr<ILogSink>> m_sinks;
    std::mutex m_mutex;
    LogLevel m_minLevel = LogLevel::Trace;
};
```

### 5.3 日志格式化

```cpp
// 使用 fmtlib (通过 {fmt}) 实现类型安全的格式化
// 避免 printf 系列的不安全格式字符串

LOG_INFO("Renderer", "初始化后端: {0} (版本 {1}.{2})", 
         "Vulkan", 1, 3);
// 输出: [Info] [Renderer] 初始化后端: Vulkan (版本 1.3)
```

### 5.4 Assertion 与 Contract 检查

```cpp
// DEBUG 模式下定义的断言宏
#if defined(PRISMA_DEBUG)
    #define PRISMA_ASSERT(condition, ...) \
        if (!(condition)) { \
            Logger::Get().Log(LogLevel::Critical, "Assert", __VA_ARGS__); \
            __debugbreak();  // 或 abort()
        }
#else
    #define PRISMA_ASSERT(condition, ...)  // Release 模式展开为空
#endif
```

### 5.5 实现步骤指南

```cpp
// 第1步: 定义 LogLevel 枚举
enum class LogLevel { Trace, Debug, Info, Warn, Error, Critical, Off };

// 第2步: 定义 ILogSink 接口
class ILogSink {
public:
    virtual ~ILogSink() = default;
    virtual void Write(LogLevel level, const std::string& message) = 0;
};

// 第3步: 实现 ConsoleSink (带 ANSI 颜色)
class ConsoleSink : public ILogSink {
    void Write(LogLevel level, const std::string& message) override {
        const char* color = "";
        switch (level) {
            case LogLevel::Trace: color = "\033[90m"; break;   // 灰色
            case LogLevel::Info:  color = "\033[92m"; break;   // 绿色
            case LogLevel::Warn:  color = "\033[93m"; break;   // 黄色
            case LogLevel::Error: color = "\033[91m"; break;   // 红色
            default: break;
        }
        fprintf(stdout, "%s%s\033[0m\n", color, message.c_str());
        fflush(stdout);
    }
};

// 第4步: 实现 Logger (单例 + 线程安全)
class Logger {
    std::mutex m_mutex;
    std::vector<std::unique_ptr<ILogSink>> m_sinks;
    
public:
    void Log(LogLevel level, const std::string& msg) {
        std::lock_guard lock(m_mutex);
        for (auto& sink : m_sinks)
            sink->Write(level, msg);
    }
};

// 第5步: 定义宏
#define LOG_INFO(category, ...) \
    Logger::Get().Log(LogLevel::Info, category, __VA_ARGS__)
```
# PrismaEngine 引擎开发教学手册 Part 2
## 核心引擎层

---

## 第6章 引擎核心架构

### 6.1 设计理念

> **"引擎是系统编排者，而非上帝对象"**

PrismaEngine 的核心架构围绕几个关键理念设计：

1. **子系统模式**: 每个主要功能（渲染、物理、音频、动画等）都是 `ISubSystem` 的独立实现
2. **组合而非继承**: Engine 持有子系统的指针，而非继承子系统
3. **快车道访问**: 频繁访问的子系统通过 Engine::Get()->GetXXX() 直接获取，避免动态查找
4. **生命周期统一**: 所有子系统共享 `Initialize → Update → Shutdown` 生命周期

### 6.2 Engine 类

`src/engine/app/Engine.h` 是引擎的绝对核心：

```cpp
class ENGINE_API Engine {
public:
    Engine(const EngineSpecification& spec);
    ~Engine();

    // 核心生命周期
    int Initialize();                           // 初始化所有子系统
    int Run(std::unique_ptr<Application> app);  // 启动主循环
    void Shutdown();                            // 关闭所有子系统

    static Engine& Get();                       // 全局单例访问

    // === 快车道访问 (系统指针缓存) ===
    Window*         GetWindow();
    RenderSystem*    GetRenderSystem();
    InputManager*    GetInputManager();
    AssetManager*    GetAssetManager();
    SceneManager*    GetSceneManager();
    PhysicsSystem*   GetPhysicsSystem();
    JobSystem*       GetJobSystem();
    AudioDevice*     GetAudioDevice();
    MemorySystem*    GetMemorySystem();
    EntityManager&   GetEntityManager();
    // ... 20+ Getter 方法

    // === 通用系统管理 ===
    template<typename T>
    T* GetSystem();  // 运行时按类型查找子系统

    template<typename T, typename... Args>
    T* AddSystem(Args&&... args);  // 动态添加额外子系统

    // === 主线程任务提交 ===
    void SubmitToMainThread(std::function<void()>&& func);

    // === 帧统计 ===
    struct FrameStats { /* ... */ };
    const FrameStats& GetFrameStats() const;
    float GetFPS() const;

private:
    void Update(Timestep ts);       // 主循环更新
    void ExecuteMainThreadQueue();  // 执行主线程队列

    EngineSpecification m_Spec;
    std::vector<std::unique_ptr<ISubSystem>> m_Systems;  // 子系统列表
    std::unique_ptr<Application> m_CurrentApp;
    std::unique_ptr<Window> m_Window;

    // 子系统指针缓存 (快车道)
    RenderSystem*   m_RenderSystem   = nullptr;
    PhysicsSystem*  m_PhysicsSystem  = nullptr;
    JobSystem*      m_JobSystem      = nullptr;
    // ... 更多子系统指针
};
```

### 6.3 ISubSystem 接口

```cpp
class ISubSystem {
public:
    virtual ~ISubSystem() = default;
    virtual int  Initialize() = 0;        // 初始化 (0=成功)
    virtual void Shutdown() = 0;          // 关闭
    virtual void Update(Timestep ts) {}    // 每帧更新 (可选)
    virtual const char* GetName() const = 0; // 子系统名称 (用于日志)
};
```

**子系统生命周期**:
```
Engine::Initialize()
  ├── 创建 Window
  ├── 初始化 Logger
  ├── 初始化 MemorySystem
  ├── 初始化 JobSystem
  ├── 初始化 AssetManager
  ├── 初始化 RenderSystem
  ├── 初始化 InputManager
  ├── 初始化 PhysicsSystem
  ├── 初始化 AudioSystem
  ├── 初始化 SceneManager
  └── 初始化 Scripting...
  
Engine::Run()
  └── 主循环:
       ├── Window::PollEvents()
       ├── ExecuteMainThreadQueue()
       ├── for (auto& system : m_Systems)
       │     system->Update(deltaTime);
       ├── m_CurrentApp->OnUpdate(deltaTime);
       ├── m_CurrentApp->OnRender();
       └── Window::SwapBuffers()
       
Engine::Shutdown()
  └── 反向顺序关闭所有子系统
```

### 6.4 引擎规范 (EngineSpecification)

```cpp
struct EngineSpecification {
    const char* Name = "Prisma Engine";
    bool Headless = false;
    bool RefreshAssetDatabaseOnStartup = false;
    LogLevel MinLogLevel = LogLevel::Trace;
    uint32_t MaxFPS = 0;
    PresentMode PresentMode = PresentMode::VSync;
    
    // CLI 覆盖
    uint32_t HeadlessFrames = 0;
    uint32_t HeadlessWidth = 0;
    uint32_t HeadlessHeight = 0;
};
```

### 6.5 Application 类

Application 是**用户代码的入口点**。引擎用户创建一个继承 Application 的类，实现虚方法：

```cpp
class Application {
public:
    Application(const ApplicationSpecification& spec);
    virtual ~Application();

    static Application& Get();

    // === 用户必须重写的生命周期钩子 ===
    virtual int OnInitialize() = 0;               // 应用初始化
    virtual void OnShutdown();                     // 应用关闭
    virtual void OnUpdate(Timestep ts);            // 每帧更新
    virtual void OnRender();                       // 每帧渲染
    virtual void OnImGuiRender();                  // ImGui 渲染
    virtual void OnEvent(Event& e);                // 事件处理

    // === 状态控制 ===
    void Close();
    bool IsRunning() const;

    // === 层管理 ===
    void PushLayer(Layer* layer);
    void PushOverlay(Layer* overlay);
    LayerStack& GetLayerStack();
};
```

### 6.6 主循环流程

```cpp
// src/engine/app/Engine.cpp 中的主循环:
int Engine::Run(std::unique_ptr<Application> app) {
    m_CurrentApp = std::move(app);
    m_Running = true;
    
    // 调用 Application::OnInitialize()
    m_CurrentApp->OnInitialize();
    
    while (m_Running) {
        // 1. 计算帧时间
        Timestep deltaTime = CalculateDeltaTime();
        
        // 2. 处理窗口事件/输入
        m_Window->PollEvents();
        
        // 3. 执行主线程任务队列
        ExecuteMainThreadQueue();
        
        // 4. 更新所有子系统
        Update(deltaTime);
        
        // 5. 更新 Application (用户代码)
        m_CurrentApp->OnUpdate(deltaTime);
        
        // 6. 渲染
        m_CurrentApp->OnRender();
        
        // 7. 呈现
        m_Window->OnUpdate();
    }
    
    m_CurrentApp->OnShutdown();
    Shutdown();
    return 0;
}
```

### 6.7 Entry Point 宏

PrismaEngine 通过宏隐藏 `main/WinMain` 入口点:

```cpp
// src/engine/EntryPoint.h
extern std::unique_ptr<Application> CreateApplication();

int main(int argc, char** argv) {
    EngineSpecification spec;
    // 解析命令行参数...
    
    Engine engine(spec);
    engine.Initialize();
    
    auto app = CreateApplication();    // 用户定义
    return engine.Run(std::move(app));
}

// 用户使用:
// class MyApp : public Application { ... };
// Application* CreateApplication() { return new MyApp(); }
```

### 6.8 实现步骤指南

```cpp
// 第1步: 定义 ISubSystem 基类
class ISubSystem {
public:
    virtual ~ISubSystem() = default;
    virtual int Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual void Update(Timestep) {}
    virtual const char* GetName() const = 0;
};

// 第2步: 实现 Engine 核心类
// 持有 subsystems vector 和快车道指针
// Initialize: 按依赖顺序创建所有子系统
// Run: 主循环 (事件处理 → 更新 → 渲染 → 呈现)
// Shutdown: 反向顺序关闭

// 第3步: 定义 Application 基类
// 提供 OnInitialize/OnUpdate/OnRender 等虚方法
// 用户派生并实现自定义逻辑

// 第4步: 定义 EntryPoint.h
// 包含 main() 函数模板
// 用户只需定义 CreateApplication()

// 第5步: 创建引擎实例的典型用法
int main() {
    Engine engine(EngineSpecification{});
    engine.Initialize();
    
    auto app = std::make_unique<MyGame>();
    return engine.Run(std::move(app));
}
```

---

## 第7章 事件系统

### 7.1 设计理念

> **"事件是引擎的神经元——连接输入、窗口、UI 和游戏逻辑"**

事件系统的设计目标:
1. **类型安全**: 每个事件是一个强类型类，而非通用字符串/字典
2. **阻塞传播**: 事件可以被消费 (`Event::Handled = true`)，阻止进一步传播
3. **零虚表开销关键路径**: 事件调度使用模板分发，避免运行时类型识别
4. **可扩展**: 添加新事件只需创建新类

### 7.2 事件定义

```cpp
namespace Prisma {

// 事件类型枚举
enum class EventType {
    None = 0,
    WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved,
    AppTick, AppUpdate, AppRender,
    KeyPressed, KeyReleased, KeyTyped,
    MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled
};

// 事件分类 (位掩码，一个事件可属于多个分类)
enum EventCategory {
    None            = 0,
    EventCategoryApplication = 1 << 0,
    EventCategoryInput       = 1 << 1,
    EventCategoryKeyboard    = 1 << 2,
    EventCategoryMouse       = 1 << 3,
    EventCategoryMouseButton = 1 << 4
};

// 快捷宏
#define EVENT_CLASS_TYPE(type) \
    static EventType GetStaticType() { return EventType::type; } \
    virtual EventType GetEventType() const override { return GetStaticType(); } \
    virtual const char* GetName() const override { return #type; }

#define EVENT_CLASS_CATEGORY(category) \
    virtual int GetCategoryFlags() const override { return category; }

// 事件基类
class Event {
public:
    bool Handled = false;       // 是否已被处理
    void* NativeEvent = nullptr; // 平台原生事件 (用于低级访问)
    
    virtual EventType GetEventType() const = 0;
    virtual const char* GetName() const = 0;
    virtual int GetCategoryFlags() const = 0;
    virtual std::string ToString() const { return GetName(); }
    
    bool IsInCategory(EventCategory category) {
        return GetCategoryFlags() & category;
    }
};

// 事件分发器 (模板分发)
class EventDispatcher {
public:
    EventDispatcher(Event& event) : m_Event(event) {}
    
    template<typename T, typename F>
    bool Dispatch(const F& func) {
        if (m_Event.GetEventType() == T::GetStaticType()) {
            m_Event.Handled |= func(static_cast<T&>(m_Event));
            return true;
        }
        return false;
    }
private:
    Event& m_Event;
};

// 具体事件示例: 窗口大小改变
class WindowResizeEvent : public Event {
public:
    WindowResizeEvent(unsigned int width, unsigned int height) 
        : m_Width(width), m_Height(height) {}
    
    unsigned int GetWidth() const { return m_Width; }
    unsigned int GetHeight() const { return m_Height; }
    
    EVENT_CLASS_TYPE(WindowResize)
    EVENT_CLASS_CATEGORY(EventCategoryApplication)
    
    std::string ToString() const override {
        return "WindowResizeEvent: " + std::to_string(m_Width) + "x" + std::to_string(m_Height);
    }
    
private:
    unsigned int m_Width, m_Height;
};

} // namespace Prisma
```

### 7.3 事件使用模式

```cpp
// 在 Application 中处理事件
void MyGame::OnEvent(Event& e) {
    EventDispatcher dispatcher(e);
    
    dispatcher.Dispatch<WindowCloseEvent>([](auto&) {
        // 处理窗口关闭
        Application::Get().Close();
        return true;  // 事件已处理
    });
    
    dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& e) {
        if (e.GetKeyCode() == KEY_ESCAPE) {
            Application::Get().Close();
            return true;
        }
        return false;
    });
}
```

### 7.4 事件在各层的流动

```
SDL3/GLFW/平台层
    ↓ 原生事件
Window::PollEvents()
    ↓ 转换为 Prisma::Event
Application::OnEvent(Event&)
    ↓ 经过 LayerStack (从顶层到底层)
Layer::OnEvent(Event&)
    ↓ 如果未被消费
继续传播到下个 Layer
```

### 7.5 实现步骤指南

```cpp
// 第1步: 定义 EventType 枚举和 Event 基类
enum class EventType { /* ... */ };

class Event {
public:
    bool Handled = false;
    virtual EventType GetEventType() const = 0;
    virtual const char* GetName() const = 0;
};

// 第2步: 定义事件分发宏
#define EVENT_CLASS_TYPE(type) /* ... */

// 第3步: 实现具体事件类 (每类事件一个类)
class KeyPressedEvent : public Event { /* ... */ };
class MouseMovedEvent : public Event { /* ... */ };

// 第4步: 实现 EventDispatcher (模板匹配 + 静态类型分发)
template<typename T, typename F>
bool Dispatch(const F& func) {
    if (m_Event.GetEventType() == T::GetStaticType()) {
        m_Event.Handled |= func(static_cast<T&>(m_Event));
        return true;
    }
    return false;
}

// 第5步: 在 Application::OnEvent() 中使用 EventDispatcher
// 或先由 LayerStack::OnEvent() 依次传递给所有层
```

---

## 第8章 层（Layer）系统

### 8.1 设计理念

> **"层是引擎的插件系统——无需修改核心即可扩展功能"**

Layer 系统允许用户将功能拆分为独立模块：
- ImGui 覆盖层 (Overlay)
- 场景层级
- 调试 HUD
- 每个层独立更新、渲染、处理事件

### 8.2 Layer 定义

```cpp
class Layer {
public:
    Layer(const std::string& name = "Layer");
    virtual ~Layer();

    virtual void OnAttach() {}       // 添加到 LayerStack 时调用
    virtual void OnDetach() {}       // 从 LayerStack 移除时调用
    virtual void OnUpdate(Timestep) {}  // 每帧更新
    virtual void OnRender() {}       // 每帧渲染
    virtual void OnImGuiRender() {}  // ImGui 渲染
    virtual void OnEvent(Event&) {}  // 事件处理

    const std::string& GetName() const;
};
```

### 8.3 LayerStack

```cpp
class LayerStack {
public:
    LayerStack();
    ~LayerStack();

    void PushLayer(Layer* layer);    // 添加到常规层末尾
    void PushOverlay(Layer* overlay);// 添加到覆盖层末尾 (最后渲染)
    void PopLayer(Layer* layer);
    void PopOverlay(Layer* overlay);

    // 更新所有层
    void OnUpdate(Timestep ts);
    void OnRender();
    void OnImGuiRender();
    void OnEvent(Event& e);  // 从顶层向底层传播
    
private:
    std::vector<Layer*> m_Layers;
    std::vector<Layer*>::iterator m_LayerInsert;  // 分隔常规层和覆盖层
};
```

### 8.4 典型使用

```cpp
class MyGame : public Application {
    int OnInitialize() override {
        PushLayer(new GameLayer());     // 游戏逻辑层
        PushOverlay(new ImGuiLayer());  // ImGui 覆盖层 (绘制在最上层)
        return 0;
    }
};
```

### 8.5 实现步骤指南

```cpp
// 第1步: 定义 Layer 基类
class Layer {
protected:
    std::string m_DebugName;
public:
    virtual void OnAttach() {}
    virtual void OnDetach() {}
    virtual void OnUpdate(Timestep) {}
    virtual void OnRender() {}
    virtual void OnImGuiRender() {}
    virtual void OnEvent(Event&) {}
};

// 第2步: 实现 LayerStack
// PushLayer: 在 m_LayerInsert 迭代器位置插入 (常规层之前)
// PushOverlay: push_back (常规层之后)
// OnEvent: 反向迭代 (最先通知覆盖层)

// 第3步: 在 Application 中使用
// m_LayerStack.OnUpdate(dt);
// m_LayerStack.OnRender();
// m_LayerStack.OnEvent(e);
```

---

## 第9章 资产管理

### 9.1 设计理念

> **"资源是引擎的血液——统一、高效、线程安全"**

AssetManager 管理系统中的资源生命周期:
1. **统一加载接口**: `Load<T>(path)` 自动处理类型分派
2. **缓存**: 相同路径只加载一次
3. **异步加载**: 通过 JobSystem 在后台线程加载
4. **资源搜索路径**: 支持多个搜索目录

### 9.2 Asset 基类

```cpp
class Asset {
public:
    Asset() = default;
    virtual ~Asset() = default;
    
    virtual bool Load(const std::filesystem::path& path) = 0;
    virtual void Unload() = 0;
    
    void SetName(const std::string& name) { m_Name = name; }
    void SetPath(const std::filesystem::path& path) { m_Path = path; }
    
    bool IsLoaded() const { return m_Loaded; }

protected:
    std::string m_Name;
    std::filesystem::path m_Path;
    bool m_Loaded = false;
};

// 类型安全句柄
template<typename T>
class AssetHandle {
public:
    AssetHandle() = default;
    AssetHandle(std::shared_ptr<T> asset) : m_Asset(std::move(asset)) {}
    
    T* operator->() { return m_Asset.get(); }
    const T* operator->() const { return m_Asset.get(); }
    T& operator*() { return *m_Asset; }
    explicit operator bool() const { return m_Asset != nullptr; }
    
private:
    std::shared_ptr<T> m_Asset;
};
```

### 9.3 AssetManager 实现

```cpp
class AssetManager : public ISubSystem {
public:
    int Initialize() override;
    void Shutdown() override;
    void Update(Timestep) override;

    // 路径管理
    void AddSearchPath(const std::filesystem::path& path);
    std::optional<std::filesystem::path> FindResource(
        const std::string& relativePath) const;

    // 同步加载 (自动缓存)
    template<typename T, typename... Args>
    AssetHandle<T> Load(const std::string& relativePath, Args&&... args) {
        StringHash hash(relativePath);
        
        // 检查缓存
        auto cached = GetCachedAsset<T>(hash);
        if (cached) return cached;
        
        // 查找文件
        auto fullPath = FindResource(relativePath);
        if (!fullPath) { /* 错误处理 */ return {}; }
        
        // 加载
        auto asset = std::make_shared<T>(std::forward<Args>(args)...);
        asset->SetName(relativePath);
        if (!asset->Load(*fullPath)) { /* 错误处理 */ return {}; }
        
        // 注册缓存
        RegisterAsset(hash, asset);
        return AssetHandle<T>(asset);
    }

    // 异步加载 (通过 JobSystem)
    template<typename T, typename... Args>
    void LoadAsync(const std::string& relativePath,
                   std::function<void(AssetHandle<T>)> callback, Args... args) {
        // 检查缓存后，通过 Engine::Get().GetJobSystem()->SubmitJob() 在后台加载
        // 加载完成后在主线程调用回调
    }

private:
    std::shared_ptr<Asset> GetAssetFromCache(StringHash hash);
    void RegisterAsset(StringHash hash, std::shared_ptr<Asset> asset);
    
    struct Impl;
    std::unique_ptr<Impl> m_Impl;  // PIMPL 隐藏实现细节
};
```

### 9.4 AssetDatabase

额外的元数据数据库，用于：
- 记录所有已知资源的元数据
- 支持资源搜索
- 支持编辑器中的资源浏览器

### 9.5 实现步骤指南

```cpp
// 第1步: 定义 Asset 基类
class Asset {
public:
    virtual bool Load(const std::filesystem::path& path) = 0;
    bool IsLoaded() const { return m_Loaded; }
protected:
    void SetLoaded(bool loaded) { m_Loaded = loaded; }
    bool m_Loaded = false;
};

// 第2步: 实现 AssetManager (ISubSystem)
// - 维护 search paths
// - 资源缓存 (unordered_map<StringHash, shared_ptr<Asset>>)
// - 同步/异步加载

// 第3步: 为每种资源类型实现 Asset 派生类
class TextureAsset : public Asset {
    bool Load(const std::filesystem::path& path) override;
    // 纹理数据...
};

// 第4步: 使用 StringHash 作为缓存键
// STL unordered_map 对 string 的哈希开销较高
// 使用 xxHash/MurmurHash 的 uint64_t 作为键
```

---

## 第10章 作业系统与多线程

### 10.1 设计理念

> **"让 CPU 的每个核心都在工作"**

现代游戏引擎必须充分利用多核 CPU：
1. **作业 (Job)**: 最小工作单元，通常是 lambda 函数
2. **线程池**: 预先创建 N 个线程（N = CPU 核心数 - 1），等待作业
3. **工作窃取**: 空闲线程可以处理其他池的作业
4. **依赖管理**: 一个作业可以在多个作业完成后执行

### 10.2 JobSystem 架构

```cpp
class JobSystem : public ISubSystem {
public:
    using Job = std::function<void()>;

    int Initialize() override;
    void Shutdown() override;
    void Update(Timestep) override;

    // 提交作业到指定线程池
    void SubmitJob(Job job, uint32_t threadPoolIndex = 0);
    
    // 等待
    void WaitForAllJobs();
    void WaitForPool(uint32_t poolIndex);

    // 线程池管理
    uint32_t RegisterPool(const std::string& name, uint32_t threadCount = 0);
    
private:
    struct ThreadPool {
        std::vector<std::thread> threads;
        std::queue<Job> jobQueue;
        mutable std::mutex queueMutex;
        std::condition_variable condition;
        std::atomic<bool> running{false};
        std::atomic<uint32_t> activeJobs{0};
        std::string name;
        
        void WorkerThread(uint32_t poolIndex, JobSystem* system);
    };
    
    std::vector<std::unique_ptr<ThreadPool>> m_threadPools;
};

// 便捷宏
#define SUBMIT_JOB(job) Engine::Get().GetJobSystem()->SubmitJob(job)
#define SUBMIT_JOB_TO_POOL(job, poolIndex) /* ... */
```

### 10.3 WorkerThread (专用工作线程)

用于需要持续处理的后台任务（如物理模拟）：

```cpp
class WorkerThread {
public:
    WorkerThread();
    ~WorkerThread();
    
    void Start(const std::string& name);
    void Stop();
    bool IsRunning() const { return m_Running; }
    
    // 设置每帧执行的回调
    void SetUpdateCallback(std::function<void(double)> callback);
    
private:
    std::thread m_Thread;
    std::atomic<bool> m_Running{false};
    std::function<void(double)> m_Callback;
    
    void ThreadLoop();
};
```

### 10.4 线程池配置

```cpp
struct PoolConfig {
    std::string name;
    uint32_t threadCount;  // 0 = auto (核心数 - 1)
    int priority;
};
```

### 10.5 线程安全策略

PrismaEngine 的线程安全设计：

| 子系统 | 线程安全性 | 说明 |
|--------|-----------|------|
| RenderSystem | 仅主线程 | GPU API 通常不是线程安全的 |
| PhysicsSystem | 单 worker 线程 | 物理步在专用线程 |
| AssetManager | 主线程 + 加载线程 | 异步加载在后台，回调在主线程 |
| JobSystem | 全线程安全 | 作业可从任意线程提交 |
| Logger | 全线程安全 | 通过 mutex 保护 |
| EntityManager | 主线程 + 读取 | 双缓冲保护 |

### 10.6 实现步骤指南

```cpp
// 第1步: 实现 ThreadPool 内部结构
struct ThreadPool {
    std::vector<std::thread> threads;
    std::queue<std::function<void()>> jobs;
    std::mutex mutex;
    std::condition_variable cv;
    std::atomic<bool> stop{false};
    
    void Worker() {
        while (true) {
            std::function<void()> job;
            {
                std::unique_lock lock(mutex);
                cv.wait(lock, [this] { return stop || !jobs.empty(); });
                if (stop && jobs.empty()) return;
                job = std::move(jobs.front());
                jobs.pop();
            }
            job();  // 执行作业
        }
    }
};

// 第2步: JobSystem::Initialize()
// 根据 CPU 核心数创建线程池
// 默认: 1 个通用池 (核心数-1 个线程)

// 第3步: 作业提交
void JobSystem::SubmitJob(Job job, uint32_t poolIndex) {
    auto& pool = m_threadPools[poolIndex];
    {
        std::lock_guard lock(pool->queueMutex);
        pool->jobQueue.push(std::move(job));
    }
    pool->condition.notify_one();
}

// 第4步: 等待机制
void JobSystem::WaitForPool(uint32_t poolIndex) {
    auto& pool = m_threadPools[poolIndex];
    while (pool->activeJobs > 0 || !pool->jobQueue.empty()) {
        std::this_thread::yield();
    }
}
```

---

## 第11章 性能分析系统

### 11.1 设计理念

> **"你不能优化你无法测量的东西"**

Profiler 系统需要：
1. **极低开销**: 分析本身不能成为瓶颈
2. **多平台**: CPU 和 GPU 时间线
3. **可视化**: 图形化的性能数据（通常通过 ImGui）

### 11.2 系统组成

```
profiling/
├── ProfilerCPU.h        # CPU 性能分析 (计时器)
├── ProfilerGPU.cpp/h    # GPU 性能分析 (查询时间戳)
├── ProfilerMacros.h     # 便捷宏
├── ProfilerSystem.h/cpp # 作为 ISubSystem 运行
├── ProfilerPanel.h/cpp  # ImGui 可视化
```

### 11.3 CPU 性能分析

```cpp
// ProfilerMacros.h
#if defined(PRISMA_ENABLE_PROFILING)
    #define PRISMA_PROFILE_SCOPE(name) ProfilerCPU profiler##__LINE__(name, __FILE__, __LINE__)
    #define PRISMA_PROFILE_FUNCTION() PRISMA_PROFILE_SCOPE(__FUNCTION__)
#else
    #define PRISMA_PROFILE_SCOPE(name)
    #define PRISMA_PROFILE_FUNCTION()
#endif
```

### 11.4 GPU 性能分析

```cpp
class ProfilerGPU {
public:
    void BeginFrame();
    void EndFrame();
    
    // 在命令缓冲区中插入时间戳
    void BeginTimestamp(VkCommandBuffer cmd, const std::string& name);
    void EndTimestamp(VkCommandBuffer cmd);
    
    // 获取结果
    struct GPUProfileData {
        std::string name;
        float durationMs;
    };
    const std::vector<GPUProfileData>& GetFrameData() const;
};
```

### 11.5 ProfilerSystem

```cpp
class ProfilerSystem : public ISubSystem {
public:
    int Initialize() override;
    void Shutdown() override;
    void Update(Timestep ts) override;
    
    // CPU 分析帧开始/结束
    void BeginFrame();
    void EndFrame();
    
    // 获取帧数据
    const FrameProfileData& GetLastFrameData() const;
    float GetFrameTime() const;
    float GetCPUTime() const;
    float GetGPUTime() const;
};

// 典型面板显示:
// ┌──────────────────────────────┐
// │ Performance Stats            │
// │ FPS: 144.2 (6.93ms)          │
// │ CPU: 3.2ms  GPU: 4.1ms      │
// │ Draw Calls: 1852             │
// │ Triangles: 2.4M              │
// │ VRAM: 1.2GB / 8.0GB         │
// └──────────────────────────────┘
```

### 11.6 实现步骤指南

```cpp
// 第1步: 定义 RAII 作用域分析器
class ProfilerCPU {
public:
    ProfilerCPU(const char* name, const char* file, uint32_t line) {
        m_Start = std::chrono::high_resolution_clock::now();
    }
    ~ProfilerCPU() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<float, std::milli>(end - m_Start).count();
        // 记录到 ProfilerSystem
    }
private:
    std::chrono::high_resolution_clock::time_point m_Start;
};

// 第2步: 在代码中使用宏
void RenderSystem::Update(Timestep ts) {
    PRISMA_PROFILE_FUNCTION();  // 自动捕获函数名
    // ...
}
```
# PrismaEngine 引擎开发教学手册 Part 3
## 渲染管线

---

## 第12章 渲染API抽象层

### 12.1 设计理念

> **"用接口隔离后端，让高层渲染逻辑与图形API无关"**

图形API（Vulkan/DX12）差异巨大，但游戏引擎需要统一的渲染抽象。PrismaEngine 的解决方案是**纯虚接口层**：

```
高层代码 (Renderer, RenderGraph, Pipeline)
    │ 仅通过接口指针调用
    ▼
IRenderDevice, ICommandBuffer, ITexture, IBuffer, ...
    ▲
    │ 派生实现
    │
VulkanRenderDevice    DirectX12RenderDevice (WIP)
```

**核心接口** (均在 `src/engine/graphic/interfaces/` 中):

### 12.2 IRenderDevice — 渲染设备

```cpp
class IRenderDevice {
public:
    virtual ~IRenderDevice() = default;
    
    // 生命周期
    virtual int Initialize(const DeviceDesc& desc) = 0;
    virtual void Shutdown() = 0;
    
    // 查询
    virtual std::string GetName() const = 0;
    virtual std::string GetAPIName() const = 0;  // "Vulkan", "DirectX12"
    virtual std::string GetGPUName() const = 0;
    
    // 命令缓冲区管理
    virtual std::unique_ptr<ICommandBuffer> CreateCommandBuffer(CommandBufferType type) = 0;
    virtual void SubmitCommandBuffer(ICommandBuffer* cmd, IFence* fence = nullptr) = 0;
    
    // 同步
    virtual void WaitForIdle() = 0;
    virtual std::unique_ptr<IFence> CreateFence() = 0;
    
    // 交换链
    virtual std::unique_ptr<ISwapChain> CreateSwapChain(...) = 0;
    
    // 帧管理
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void Present() = 0;
    virtual void Resize(uint32_t width, uint32_t height) = 0;
    
    // 功能查询
    virtual bool SupportsBindlessTextures() const = 0;
    virtual bool SupportsRayTracing() const = 0;
    virtual bool SupportsComputeShader() const = 0;
    virtual bool SupportsMeshShader() const = 0;
    
    // 统计
    virtual RenderStats GetRenderStats() const = 0;
    
    // 资源工厂
    virtual IResourceFactory* GetResourceFactory() const = 0;
    
    // Vulkan 特定 (供需要直接访问 Vulkan API 的代码)
    virtual VkInstance       GetVkInstance() const = 0;
    virtual VkDevice         GetVkDevice() const = 0;
    virtual VmaAllocator     GetVmaAllocator() const = 0;
    // ...
};
```

### 12.3 其他关键接口

**ICommandBuffer** — 命令录制:
```cpp
class ICommandBuffer {
public:
    virtual void BeginRecording() = 0;
    virtual void EndRecording() = 0;
    virtual void BeginRenderPass(IRenderTarget* rt, IDepthStencil* ds) = 0;
    virtual void EndRenderPass() = 0;
    virtual void SetPipelineState(IPipelineState* state) = 0;
    virtual void SetVertexBuffer(IBuffer* buffer, uint32_t slot) = 0;
    virtual void SetIndexBuffer(IBuffer* buffer, bool is32Bit) = 0;
    virtual void Draw(uint32_t vertexCount, uint32_t startVertex) = 0;
    virtual void DrawIndexed(uint32_t indexCount, uint32_t startIndex, int32_t baseVertex) = 0;
    virtual void DrawIndexedInstanced(...) = 0;
    virtual void Dispatch(uint32_t x, uint32_t y, uint32_t z) = 0;  // Compute
    virtual void PipelineBarrier(...) = 0;
    virtual void Reset() = 0;
};
```

**ITexture/IBuffer/ISampler/IShader** — GPU 资源抽象:
```cpp
class ITexture {
    virtual void* GetNativeHandle() const = 0;
    virtual TextureDesc GetDesc() const = 0;
    virtual ResourceId GetResourceId() const = 0;
};

class IBuffer {
    virtual void* GetNativeHandle() const = 0;
    virtual BufferDesc GetDesc() const = 0;
    virtual void UpdateData(const void* data, uint64_t size, uint64_t offset) = 0;
};

class IPipeline {
    virtual int Initialize(IRenderDevice* device) = 0;
    virtual void Shutdown() = 0;
    virtual void Execute(const RenderContext& ctx) = 0;
};
```

### 12.4 类型系统

`src/engine/graphic/interfaces/RenderTypes.h` 定义所有渲染相关的枚举和结构:

```cpp
namespace Prisma::Graphic {

enum class RenderAPIType { None, DirectX12, Vulkan, OpenGL };
enum class BufferType { Unknown, Vertex, Index, Constant, Structured, Raw };
enum class TextureType { Texture2D, Texture3D, TextureCube };
enum class TextureFormat { RGBA8_UNorm, RGBA16_Float, D32_Float, BC3_UNorm, ... };
enum class ShaderType { Vertex, Pixel, Geometry, Compute, ... };
enum class PresentMode { Immediate, VSync, Mailbox, Adaptive };
enum class PrimitiveTopology { TriangleList, TriangleStrip, ... };
enum class FillMode { Wireframe, Solid };
enum class CullMode { None, Front, Back };
enum class BlendOp { Add, Subtract, ... };

struct Vertex { Vector4 position, color, uv, normal, texCoord, tangent; };
struct Light { Vector4 position, color, direction; };
struct BoundingBox { Vector3 minBounds, maxBounds; };
struct CameraData { Matrix4x4 viewMatrix, projectionMatrix; Vector3 position; ... };

} // namespace Prisma::Graphic
```

### 12.5 RenderSystem — 渲染子系统

`RenderSystem` 是 `ISubSystem` 实现，连接引擎和渲染后端:

```cpp
class RenderSystem : public ISubSystem {
public:
    int Initialize() override;
    void Shutdown() override;
    void Update(Timestep ts) override;

    // 帧控制
    void BeginFrame();
    void EndFrame();
    void Present();
    void Resize(uint32_t width, uint32_t height);

    // 设备访问
    IRenderDevice* GetDevice() const;
    IRenderResourceManager* GetRenderResourceManager() const;

    // 渲染管线
    void SetMainPipeline(std::shared_ptr<IPipeline> pipeline);
    IPipeline* GetMainPipeline() const;

    // 场景渲染
    void RenderScene(Scene* scene, ICamera* camera, ITexture* target = nullptr);

private:
    int InitializeDevice();       // 创建 Vulkan/DX12 设备
    int InitializeRenderResourceManager();  // 资源管理器
    int InitializeRenderPipelines();  // 创建默认管线

    RenderSystemDesc m_desc;
    std::unique_ptr<IRenderDevice> m_device;
    std::shared_ptr<RenderResourceManager> m_renderResourceManager;
    std::shared_ptr<IPipeline> m_mainRenderPipeline;
};
```

**Initialize() 流程**:
```
1. InitializeDevice()
   └── 创建 VulkanRenderDevice → VkInstance → VkPhysicalDevice → VkDevice → VMA

2. InitializeRenderResourceManager()
   └── 创建 RenderResourceManager → 初始化资源池

3. InitializeRenderPipelines()
   ├── 创建 ForwardPipeline (默认)
   ├── 或 ClusteredForwardPipeline
   ├── 或 PathTracingPipeline (如果有光追硬件)
   └── 或 SRP (Scriptable Render Pipeline，由 C# 接管)
```

### 12.6 RenderCommandContext

渲染命令上下文是 `IDeviceContext` 的实现，提供高层 API 封装:

```cpp
class RenderCommandContext : public IDeviceContext {
public:
    // 渲染目标
    void SetRenderTarget(IRenderTarget* rt, IDepthStencil* ds);
    void SetViewport(float x, float y, float width, float height);
    void SetScissorRect(const Rect& rect);
    
    // 管线状态
    void SetPipelineState(IPipelineState* state);
    
    // 资源绑定
    void SetVertexBuffer(IBuffer* buffer, uint32_t slot, uint32_t offset, uint32_t stride);
    void SetIndexBuffer(IBuffer* buffer, uint32_t offset, bool is32Bit);
    void SetConstantBuffer(IBuffer* buffer, uint32_t slot, uint32_t offset, uint32_t size);
    void SetTexture(ITexture* texture, uint32_t slot);
    void SetSampler(ISampler* sampler, uint32_t slot);
    
    // 渲染
    void Draw(uint32_t vertexCount, uint32_t startVertex = 0);
    void DrawIndexed(uint32_t indexCount, uint32_t startIndex = 0, int32_t baseVertex = 0);
    void DrawInstanced(...);
    void DrawIndexedInstanced(...);
    
    // 清理
    void ClearRenderTarget(IRenderTarget* rt, const float color[4]);
    void ClearDepthStencil(IDepthStencil* ds, float depth, uint8_t stencil);
    
private:
    struct StateCache { /* ... 缓存当前绑定状态 ... */ };
    StateCache m_stateCache;
};
```

### 12.7 实现步骤指南

```cpp
// 第1步: 定义所有渲染接口 (纯虚类)
// 从数据类开始: RenderTypes.h (Vertex, BoundingBox, Light, 所有枚举)
// 再到功能接口: IRenderDevice, ICommandBuffer, ITexture, IBuffer

// 第2步: 实现 RenderSystem
// 作为 ISubSystem 包装设备创建和管线管理

// 第3步: 为每个后端实现接口
// Vulkan: adapters/vulkan/RenderDeviceVulkan.cpp
// DX12: adapters/dx12/RenderDeviceDX12.cpp (开发中)

// 第4步: 通过条件编译选择后端
// cmake/DeviceOptions.cmake 控制 PRISMA_ENABLE_RENDER_VULKAN 等
```

---

## 第13章 Vulkan后端实现

### 13.1 设计理念

> **"Vulkan 的显式控制需要同样显式的封装"**

Vulkan 是当前唯一的完整后端，PrismaEngine 利用:
- **vk-bootstrap**: 简化 Vulkan 实例/设备创建
- **VMA (Vulkan Memory Allocator)**: 自动化 GPU 内存管理
- **volk**: 动态加载 Vulkan 函数指针
- **SPIRV-Reflect**: 反射 SPIR-V 获取着色器绑定信息

### 13.2 RenderDeviceVulkan

```cpp
namespace Prisma::Graphic::Vulkan {

class RenderDeviceVulkan : public IRenderDevice {
public:
    int Initialize(const DeviceDesc& desc) override;
    void Shutdown() override;
    
    // === 初始化流程 ===
    // 1. volk 初始化 (加载 Vulkan 函数)
    // 2. vkb::InstanceBuilder 创建 VkInstance
    // 3. 创建 VkSurfaceKHR (连接窗口系统)
    // 4. vkb::PhysicalDeviceSelector 选择 GPU
    // 5. vkb::DeviceBuilder 创建 VkDevice
    // 6. VMA 初始化 (VmaAllocator)
    // 7. 创建命令池和命令缓冲区
    // 8. 创建信号量/围栏
    // 9. 创建描述符池
    // 10. 创建交换链
    
private:
    // vk-bootstrap 核心
    vkb::Instance m_vkbInstance;
    vkb::PhysicalDevice m_vkbPhysicalDevice;
    vkb::Device m_vkbDevice;
    
    VkInstance      m_instance;
    VkSurfaceKHR    m_surface;
    VkPhysicalDevice m_physicalDevice;
    VkDevice        m_device;
    VmaAllocator    m_allocator;
    
    // 队列
    VkQueue m_graphicsQueue;
    VkQueue m_presentQueue;
    uint32_t m_graphicsQueueFamily;
    VkQueue m_computeQueue;
    
    // 同步
    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;
    
    // 帧索引
    uint32_t m_currentFrame = 0;
    bool m_frameActive = false;
    
    // 交换链
    std::unique_ptr<VulkanSwapChain> m_swapChain;
    
    // 资源工厂
    std::unique_ptr<VulkanResourceFactory> m_resourceFactory;
    
    // 设备功能
    struct DeviceFeatures {
        bool supportsBindless = false;
        bool supportsRayTracing = false;
        bool supportsMeshShading = false;
        bool supportsVariableRateShading = false;
    } m_deviceFeatures;
};

} // namespace Prisma::Graphic::Vulkan
```

### 13.3 Vulkan 帧循环

```cpp
void RenderDeviceVulkan::BeginFrame() {
    // 等待当前帧的围栏
    vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]);
    
    // 获取下一张交换链图像
    VkResult result = vkAcquireNextImageKHR(
        m_device, m_swapChain->GetSwapChain(), UINT64_MAX,
        m_imageAvailableSemaphores[m_currentFrame],
        VK_NULL_HANDLE, &m_imageIndex);
    
    // 重置命令缓冲区
    m_vulkanCommandBuffers[m_currentFrame]->Reset();
    m_vulkanCommandBuffers[m_currentFrame]->BeginRecording();
    
    m_frameActive = true;
}

void RenderDeviceVulkan::EndFrame() {
    m_vulkanCommandBuffers[m_currentFrame]->EndRecording();
    m_frameActive = false;
}

void RenderDeviceVulkan::Present() {
    // 提交命令缓冲区
    VkSubmitInfo submitInfo = {};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = m_commandBuffers[m_currentFrame];
    // ... 信号量链
    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]);
    
    // 呈现
    VkPresentInfoKHR presentInfo = {};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = m_swapChain->GetSwapChainPtr();
    vkQueuePresentKHR(m_presentQueue, &presentInfo);
    
    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
```

### 13.4 VulkanAdapter — 接口到 Vulkan 的映射

```cpp
class VulkanAdapter {
public:
    // 将 IBuffer 转换为 VkBuffer
    static VkBuffer GetVkBuffer(IBuffer* buffer);
    // 将 ITexture 转换为 VkImage + VkImageView
    static VkImage GetVkImage(ITexture* texture);
    static VkImageView GetVkImageView(ITexture* texture);
    // 将 ISampler 转换为 VkSampler
    static VkSampler GetVkSampler(ISampler* sampler);
    // 将 IShader 转换为 VkShaderModule
    static VkShaderModule GetVkShaderModule(IShader* shader);
    
    // 枚举转换
    static VkFormat ToVkFormat(TextureFormat format);
    static VkPrimitiveTopology ToVkTopology(PrimitiveTopology topology);
    // ... 100+ 转换函数
};
```

### 13.5 关键 Vulkan 技术选型

| 技术 | 选择 | 理由 |
|------|------|------|
| 实例/设备创建 | vk-bootstrap | 避免编写数百行初始化样板代码 |
| 内存管理 | VMA (v3.2.1) | 自动化 sub-allocation，碎片管理 |
| 着色器 | SPIR-V | 编译时从 GLSL/HLSL 编译，运行时直接加载 |
| 描述符 | 绑定模式 (push descriptor) | 减少描述符池管理开销 |
| 交换链 | 自定义包装 | 支持 headless 模式 |
| 调试 | VK_LAYER_KHRONOS_validation | Debug 构建启用，Release 禁用 |

### 13.6 实现步骤指南

```cpp
// 第1步: 使用 vk-bootstrap 创建 Vulkan 实例和物理设备
vkb::InstanceBuilder builder;
auto inst_ret = builder.set_app_name("PrismaEngine")
                      .request_validation_layers(true)
                      .build();
vkb::Instance vkb_inst = inst_ret.value();

// 第2步: 选择物理设备 (优先独立 GPU)
vkb::PhysicalDeviceSelector selector(vkb_inst);
auto phys_ret = selector.set_surface(surface)
                       .set_minimum_version(1, 3)
                       .prefer_gpu_device_type(vkb::PreferredDeviceType::Discrete)
                       .build();
vkb::PhysicalDevice phys = phys_ret.value();

// 第3步: 创建逻辑设备
vkb::DeviceBuilder device_builder(phys);
auto dev_ret = device_builder.build();
vkb::Device vkb_device = dev_ret.value();

// 第4步: 初始化 VMA
VmaAllocatorCreateInfo allocInfo = {};
allocInfo.physicalDevice = physicalDevice;
allocInfo.device = device;
allocInfo.instance = instance;
vmaCreateAllocator(&allocInfo, &m_allocator);

// 第5步: 创建命令池和命令缓冲区
VkCommandPoolCreateInfo poolInfo = {};
poolInfo.queueFamilyIndex = graphicsQueueFamily;
vkCreateCommandPool(device, &poolInfo, nullptr, &m_commandPool);

// 第6步: 创建交换链
m_swapChain = std::make_unique<VulkanSwapChain>(device, physicalDevice, 
    surface, width, height, presentMode);

// 第7步: 创建同步原语
for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    VkSemaphoreCreateInfo semInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    vkCreateSemaphore(device, &semInfo, nullptr, &m_imageAvailableSemaphores[i]);
    vkCreateSemaphore(device, &semInfo, nullptr, &m_renderFinishedSemaphores[i]);
    VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    vkCreateFence(device, &fenceInfo, nullptr, &m_inFlightFences[i]);
}
```

---

## 第14章 渲染资源管理

### 14.1 设计理念

> **"GPU 资源是稀有资源——需要跟踪、缓存、复用"**

`RenderResourceManager` 管理所有 GPU 资源的生命周期:
- **纹理**: 加载、创建、mipmap 生成
- **缓冲区**: 顶点/索引/常量/结构化缓冲区
- **着色器**: GLSL/HLSL 编译为 SPIR-V
- **管线**: 图形/计算管线状态对象 (PSO)
- **采样器**: 纹理采样状态

### 14.2 IRenderResourceManager 接口

```cpp
class IRenderResourceManager {
public:
    virtual ~IRenderResourceManager() = default;
    virtual int Initialize(IRenderDevice* device) = 0;
    virtual void Shutdown() = 0;
    
    // 纹理
    virtual std::shared_ptr<ITexture> LoadTexture(const std::string& filename, bool generateMips = true) = 0;
    virtual std::shared_ptr<ITexture> CreateTexture(const TextureDesc& desc) = 0;
    virtual std::shared_ptr<ITexture> CreateTextureFromMemory(const void* data, uint64_t dataSize, const TextureDesc& desc) = 0;
    
    // 缓冲区
    virtual std::shared_ptr<IBuffer> CreateBuffer(const BufferDesc& desc) = 0;
    virtual std::shared_ptr<IBuffer> CreateDynamicBuffer(uint64_t size, BufferType type) = 0;
    
    // 着色器
    virtual std::shared_ptr<IShader> LoadShader(const std::string& filename, ...) = 0;
    virtual std::shared_ptr<IShader> CreateShader(const std::string& source, const ShaderDesc& desc) = 0;
    
    // 管线
    virtual std::shared_ptr<IPipeline> CreatePipeline(const PipelineDesc& desc) = 0;
    virtual std::shared_ptr<IPipelineState> CreatePipelineState(const PipelineStateDesc& desc) = 0;
    
    // 采样器
    virtual std::shared_ptr<ISampler> CreateSampler(const SamplerDesc& desc) = 0;
    virtual std::shared_ptr<ISampler> GetDefaultSampler() = 0;
    
    // 生命周期
    virtual void ReleaseResource(ResourceId id) = 0;
    virtual void GarbageCollect() = 0;
    virtual void ReleaseAllResources() = 0;
    
    // 热重载
    virtual void EnableHotReload(bool enable) = 0;
    virtual void CheckAndReloadResources() = 0;
    
    // 统计
    virtual ResourceStats GetResourceStats() const = 0;
    
    // 线程安全
    virtual std::shared_mutex& GetResourceLock() = 0;
};
```

### 14.3 内部实现

```cpp
class RenderResourceManager : public IRenderResourceManager {
public:
    int Initialize(IRenderDevice* device) override;
    void Shutdown() override;
    void Update(Timestep ts);  // 处理异步加载完成、清理过期资源
    
private:
    IRenderDevice* m_device;
    std::atomic<ResourceId> m_nextId{1};
    
    // 资源存储
    std::unordered_map<ResourceId, std::shared_ptr<IResource>> m_resources;
    std::unordered_map<std::string, ResourceId> m_nameToId;
    mutable std::shared_mutex m_resourceMutex;
    
    // 异步加载队列
    std::queue<ResourceLoadTask> m_loadQueue;
    std::mutex m_loadQueueMutex;
    std::condition_variable m_loadQueueCV;
    std::thread m_loadingThread;
    
    // 热重载
    bool m_hotReloadEnabled = false;
    std::unordered_map<std::string, std::filesystem::file_time_type> m_fileTimestamps;
};
```

**异步加载工作流**:
```
LoadTextureAsync("stone.png")
    │
    ├→ 生成 ResourceId
    ├→ Push ResourceLoadTask 到 m_loadQueue
    └→ 返回 ResourceId (立即返回，不阻塞)
    
LoadingThread 取出任务:
    ├→ stbi_load 从磁盘读取图像数据
    ├→ 创建 VkImage + VkImageView (通过 VMA)
    ├→ 生成 mipmap
    └→ 调用 callback(ResourceId, shared_ptr<ITexture>)

Update() 检查完成的任务:
    ├→ 从完成队列取出
    └→ 通知等待者
```

### 14.4 热重载 (Hot-Reload)

```cpp
void RenderResourceManager::EnableHotReload(bool enable) {
    m_hotReloadEnabled = enable;
    if (enable) {
        // 记录所有已加载资源的时间戳
        for (auto& [name, id] : m_nameToId) {
            m_fileTimestamps[name] = std::filesystem::last_write_time(name);
        }
    }
}

void RenderResourceManager::CheckAndReloadResources() {
    if (!m_hotReloadEnabled) return;
    
    for (auto& [name, id] : m_nameToId) {
        auto currentTime = std::filesystem::last_write_time(name);
        if (currentTime != m_fileTimestamps[name]) {
            // 文件已修改，重新加载
            ReloadResource(name, id);
            m_fileTimestamps[name] = currentTime;
        }
    }
}
```

### 14.5 实现步骤指南

```cpp
// 第1步: 定义资源接口 IResource
class IResource {
public:
    virtual ResourceId GetId() const = 0;
    virtual const std::string& GetName() const = 0;
    virtual ResourceType GetType() const = 0;
    virtual uint64_t GetMemoryUsage() const = 0;
};

// 第2步: 实现每种资源的包装
// VulkanTexture : public ITexture { VkImage, VkImageView, VmaAllocation };
// VulkanBuffer : public IBuffer { VkBuffer, VmaAllocation };

// 第3步: 实现 RenderResourceManager
// 统一工厂方法，内部路由到 VulkanResourceFactory
// 异步加载线程 + 队列

// 第4步: 热重载支持
// 定期检查文件时间戳，自动重新加载修改的资源
```

---

## 第15章 渲染管线系统

### 15.1 设计理念

> **"管线是渲染逻辑的组织单元——每个场景的渲染方式可能不同"**

PrismaEngine 支持多种渲染管线:
- **Forward Pipeline**: 前向渲染，适用于小型场景
- **Clustered Forward**: 分簇前向渲染，数千光源支持
- **Deferred Pipeline**: 延迟渲染 (开发中)
- **Path Tracing Pipeline**: 路径追踪，基于硬件光追
- **2D Pipeline**: 2D 精灵渲染
- **SRP (Scriptable Render Pipeline)**: 由 C# 定义的完全可编程管线

### 15.2 IPipeline 接口

```cpp
class IPipeline {
public:
    virtual ~IPipeline() = default;
    
    virtual int Initialize(IRenderDevice* device) = 0;
    virtual void Shutdown() = 0;
    
    // 核心渲染方法
    virtual void Execute(const RenderContext& ctx) = 0;
    
    // 场景加载后回调
    virtual void OnSceneLoaded(Scene* scene) {}
};

// 渲染上下文 (传递给管线)
struct RenderContext {
    IRenderDevice* device = nullptr;
    ICommandBuffer* commandBuffer = nullptr;
    ITexture* targetTexture = nullptr;     // 离屏渲染目标
    
    CameraData camera;
    Vector4 clearColor = {0.1f, 0.1f, 0.1f, 1.0f};
    std::vector<Light> lights;
    
    uint32_t frameIndex = 0;
    float deltaTime = 0.0f;
};
```

### 15.3 ForwardPipeline — 前向渲染

```cpp
class ForwardPipeline : public IPipeline {
public:
    int Initialize(IRenderDevice* device) override;
    void Shutdown() override;
    void Execute(const RenderContext& ctx) override;
    
private:
    void CreateRenderPasses();
    void CreateShaders();
    void CreatePipelineStates();
    
    void BindSceneData(const RenderContext& ctx);
    void RenderMeshes(const RenderContext& ctx, const std::vector<RenderCommand>& commands);
    
    // 管线状态
    std::shared_ptr<IPipelineState> m_psoOpaque;
    std::shared_ptr<IPipelineState> m_psoTransparent;
    std::shared_ptr<IPipelineState> m_psoGizmo;
};
```

**Execute 流程**:
```
Execute(RenderContext)
  ├── BeginSwapChainRenderPass
  ├── BindSceneData (Camera UB, Light UB)
  ├── RenderMeshes Opaque
  │   ├── 按材质排序 → 区间合并
  │   ├── 对每个 RenderCommand:
  │   │   ├── BindPipelineState (对应材质)
  │   │   ├── PushConstant (transform matrix)
  │   │   ├── BindVertexBuffer
  │   │   ├── BindIndexBuffer
  │   │   ├── BindTexture/Sampler
  │   │   └── DrawIndexed
  │   └── →
  ├── RenderMeshes Transparent (排序: 从远到近)
  ├── RenderGizmo
  └── EndSwapChainRenderPass
```

### 15.4 ClusteredForwardPipeline — 分簇前向渲染

适用于大量动态光源（数千个点光源）:
```
1. 将视锥体分为 3D 网格 (簇)
2. 计算每个簇包含哪些光源 (Compute Shader)
3. 渲染时每像素读取所在簇的光源列表
4. 只计算有影响的 N 个光源 → 避免 O(N*M) 问题
```

### 15.5 PathTracingPipeline — 路径追踪

```cpp
class PathTracingPipeline : public IPipeline {
    // 基于硬件光线追踪 (VK_KHR_ray_tracing)
    // 使用 TLAS/BLAS 加速结构
    // 支持 BVH 和 BruteForce 模式
    // 支持 NEE (Next Event Estimation)
    // 支持多重重要性采样 (MIS)
    
    struct Settings {
        uint32_t maxSamples = 512;
        uint32_t maxBounces = 4;
        bool enableNEE = false;
        PathTraceMode mode = PathTraceMode::BVH;
    };
};
```

### 15.6 实现步骤指南

```cpp
// 第1步: 定义 IPipeline 接口
// Initialize → Shutdown → Execute 生命周期

// 第2步: 实现 ForwardPipeline
// - 创建 PSO (顶点着色器+像素着色器+光栅化状态+混合状态)
// - 每帧收集 RenderCommand 并执行

// 第3步: 实现更多管线
// ClusteredForward → Compute Shader 光簇分配
// PathTracing → VK_KHR_ray_tracing 加速结构

// 第4步: 管道选择
// RenderSystem 根据 RenderMode 选择默认管线
// 用户可通过 SetMainPipeline() 自定义
```

---

## 第16章 渲染图（Render Graph）

### 16.1 设计理念

> **"将渲染声明为有向无环图，让系统自动管理资源和执行顺序"**

传统渲染管道的问题是 Pass 之间的依赖关系硬编码。Render Graph:
1. **声明式 API**: Pass 声明它读/写哪些资源
2. **自动裁剪**: 未被使用的 Pass 不执行
3. **资源别名**: 生命周期不重叠的资源复用同一内存
4. **自动屏障**: 系统自动插入正确的管线屏障

### 16.2 RenderGraph 架构

```cpp
class RenderGraph {
public:
    RenderGraph(IRenderDevice* device);
    ~RenderGraph();

    // 添加一个 Pass (模板方法)
    template<typename PassData, typename SetupFunc, typename ExecuteFunc>
    void AddPass(const std::string& name, SetupFunc&& setup, ExecuteFunc&& execute) {
        uint32_t passIndex = (uint32_t)m_Passes.size();
        auto& pass = m_Passes.emplace_back();
        pass.Name = name;
        
        // 设置阶段
        RGBuilder builder(this, passIndex);
        PassData data;
        setup(builder, data);  // 声明读写资源
        
        // 执行阶段 (保存 Lambda)
        pass.Execute = [data, execute](RGContext& ctx) {
            execute(data, ctx);  // 实际渲染
        };
    }

    void Compile();   // 编译图 (排序、裁剪、分配资源)
    void Execute(RenderCommandContext* cmd);  // 执行图

private:
    struct PassNode {
        std::string Name;
        std::function<void(RGContext&)> Execute;
        std::vector<uint32_t> Inputs;
        std::vector<uint32_t> Outputs;
    };
    
    struct ResourceNode {
        RGResourceDesc Desc;
        std::string Name;
        void* NativeResource;  // 映射到 VkImage/Buffer
    };
    
    IRenderDevice* m_Device;
    std::vector<PassNode> m_Passes;
    std::vector<ResourceNode> m_Resources;
};
```

### 16.3 使用示例

```cpp
RenderGraph graph(device);

// 声明 HDR 纹理和深度纹理
auto hdrRT = graph.CreateTexture(RGResourceDesc::Texture2D(1920, 1080));
auto depthRT = graph.CreateTexture(RGResourceDesc::Texture2D(1920, 1080, D32F));

// 几何 Pass
graph.AddPass<GeometryPassData>("Geometry",
    /* Setup */  [&](RGBuilder& builder, GeometryPassData& data) {
        data.OutputColor = builder.Write(hdrRT);
        data.OutputDepth = builder.Write(depthRT);
    },
    /* Execute */ [&](const GeometryPassData& data, RGContext& ctx) {
        // 渲染几何体到 hdrRT + depthRT
    }
);

// 灯光 Pass
graph.AddPass<LightingPassData>("Lighting",
    /* Setup */  [&](RGBuilder& builder, LightingPassData& data) {
        data.InputColor = builder.Read(hdrRT);
        data.InputDepth = builder.Read(depthRT);
    },
    /* Execute */ [&](const LightingPassData& data, RGContext& ctx) {
        // 基于 hdrRT 和 depthRT 计算光照
    }
);

graph.Compile();
graph.Execute(cmd);
```

### 16.4 实现步骤指南

```cpp
// 第1步: 定义图的数据结构 (ResourceNode, PassNode)
// Resources: 描述尺寸/格式，实际 GPU 分配在 Compile 时
// Passes: Setup 声明读写资源，Execute 执行实际渲染

// 第2步: 实现 RGBuilder
// CreateTexture: 添加资源节点，返回句柄
// Read/Write: 在 Pass 的 Inputs/Outputs 中添加资源索引

// 第3步: Compile (拓扑排序)
// 构建依赖图 → 拓扑排序 → 确定执行顺序
// 裁剪未被使用的 Pass
// 分配实际 GPU 资源

// 第4步: Execute
// 按排序后的 Pass 列表依次执行
// 自动插入管线屏障
```

---

## 第17章 2D渲染系统

### 17.1 设计理念

2D 渲染是独立于 3D 渲染的子系统，用于:
- UI/菜单界面
- 2D 游戏 (Prisma2D 示例项目)
- 编辑器 Gizmo
- 调试可视化

### 17.2 2D 管线

```cpp
// Pipeline2D — 2D 渲染管线
class Pipeline2D : public IPipeline {
    void Execute(const RenderContext& ctx) override;
    // 渲染 CanvasPass2D → UIPass2D → Light2DPass → PostProcessPass2D
};

// Graphics2D — 2D 绘制 API (类似 Unity 的 Graphics.Draw)
class Graphics2D {
    static void DrawSprite(Texture* texture, const Vector2& position, const Color& color);
    static void DrawRect(const Vector2& min, const Vector2& max, const Color& color);
    static void DrawLine(const Vector2& start, const Vector2& end, const Color& color);
    static void DrawText(const std::string& text, const Vector2& position, Font* font);
};

// CanvasPass2D — 画布渲染 Pass
class CanvasPass2D : public RenderPass {
    void Execute() override;
    // 渲染所有精灵到离屏缓冲区
};

// Light2DPass — 2D 光照 Pass
class Light2DPass : public RenderPass {
    void Execute() override;
    // 使用法线贴图在 2D 画布上计算光照
};

// UIPass2D — UI Pass
class UIPass2D : public RenderPass {
    void Execute() override;
    // 渲染 UI 元素（按钮、文本、面板）
};

// PostProcessPass2D — 后处理 Pass
class PostProcessPass2D : public RenderPass {
    void Execute() override;
    // 颜色校正、辉光、模糊等
};
```

### 17.3 SpriteAnimation — 2D 动画

```cpp
class SpriteAnimation {
    std::vector<TextureRegion> frames;
    float frameDuration;
    bool looping;
    
    TextureRegion GetCurrentFrame(float time) const;
};
```

### 17.4 实现步骤指南

```cpp
// 第1步: 实现 Sprite 渲染
// 使用全屏四边形 + 纹理采样器
// 批处理 (多个精灵合并为一次 DrawCall)

// 第2步: 实现 CanvasPass2D
// 离屏渲染到中间 RT
// 支持相机变换 (滚动/缩放)

// 第3步: 2D 光照
// 法线贴图支持
// 点光源/方向光
```

---

## 第18章 相机与变换系统

### 18.1 设计理念

相机是渲染的"眼睛"。PrismaEngine 的相机设计：
- **作为 Component**: Camera 继承自 Component，挂载在场景 Node 上
- **视角/正交**: 支持透视和正交投影
- **脏标记**: 变换改变时自动重新计算矩阵

### 18.2 Camera 类

```cpp
class Camera : public Component, public ICamera {
public:
    // 投影模式
    ProjectionMode GetProjectionMode() const;
    void SetPerspective(float fov, float aspect, float near, float far);
    void SetOrthographic(float size, float aspect, float near, float far);
    
    // 矩阵获取 (脏标记自动计算)
    Matrix4x4 GetViewMatrix() const override;
    Matrix4x4 GetProjectionMatrix() const override;
    Matrix4x4 GetViewProjectionMatrix() const override;
    
    // 位置/方向
    Vector3 GetPosition() const override;
    Vector3 GetForward() const override;
    Vector3 GetUp() const override;
    
    // 控制
    void MoveWorld(const Vector3& direction);
    void MoveLocal(float forward, float right, float up);
    void Rotate(float pitch, float yaw, float roll);
    void LookAt(const Vector3& target);
    
private:
    ProjectionMode m_projectionMode = ProjectionMode::Perspective;
    float m_fov = 70.0f;
    float m_aspectRatio = 16.0f / 9.0f;
    float m_nearPlane = 0.1f;
    float m_farPlane = 1000.0f;
    
    mutable Matrix4x4 m_viewMatrix;
    mutable Matrix4x4 m_projectionMatrix;
    mutable bool m_isViewDirty = true;
    mutable bool m_isProjectionDirty = true;
};
```

### 18.3 CameraController — 相机控制

```cpp
class CameraController {
    // 鼠标拖拽旋转
    // WASD 移动
    // 滚轮缩放
    
    void Update(Timestep ts, Camera* camera, InputManager* input);
};
```

### 18.4 Transform — 变换组件

```cpp
class Transform : public Component {
public:
    Vector3 GetLocalPosition() const;
    Quaternion GetLocalRotation() const;
    Vector3 GetLocalScale() const;
    
    void SetLocalPosition(const Vector3& pos);
    void SetLocalRotation(const Quaternion& rot);
    void SetLocalScale(const Vector3& scale);
    
    void Translate(const Vector3& delta);
    void Rotate(float angle, const Vector3& axis);
    
    Matrix4x4 GetLocalMatrix() const;
    Matrix4x4 GetWorldMatrix() const;  // 递归计算父节点链
    
private:
    Vector3 m_LocalPosition = Vector3(0.0f);
    Quaternion m_LocalRotation = Quaternion(1.0f, 0.0f, 0.0f, 0.0f);
    Vector3 m_LocalScale = Vector3(1.0f);
    Node* m_Parent = nullptr;
};
```

### 18.5 ICamera 接口

```cpp
class ICamera {
public:
    virtual Matrix4x4 GetViewMatrix() const = 0;
    virtual Matrix4x4 GetProjectionMatrix() const = 0;
    virtual Matrix4x4 GetViewProjectionMatrix() const = 0;
    virtual Vector3 GetPosition() const = 0;
    virtual Vector3 GetForward() const = 0;
    virtual float GetFOV() const = 0;
    virtual float GetNearPlane() const = 0;
    virtual float GetFarPlane() const = 0;
    virtual void SetViewport(uint32_t width, uint32_t height) = 0;
};
```

### 18.6 FrustumCulling — 视锥剔除

```cpp
struct Frustum {
    Plane planes[6];  // 左,右,顶,底,近,远
};

class FrustumCullingSystem {
public:
    void Update(const Matrix4x4& viewProjection);
    
    bool IsVisible(const BoundingBox& bounds, const Matrix4x4& transform) const;
    bool IsVisible(const Vector3& point, float radius) const;
    
    struct CullingStats {
        uint32_t totalObjects;
        uint32_t culledObjects;
        uint32_t visibleObjects;
    };
    CullingStats GetStats() const;
};
```

### 18.7 实现步骤指南

```cpp
// 第1步: 实现 Transform 组件
// 位置/旋转/缩放 → 4x4 矩阵
// 脏标记: 改变时标记，访问时重新计算

// 第2步: 实现 Camera 组件
// 透视投影: Matrix4x4 = Math::Perspective(fov, aspect, near, far)
// 正交投影: Matrix4x4 = Math::Orthographic(...)
// 视图矩阵: Matrix4x4 = Math::LookAt(position, target, up)

// 第3步: 实现 CameraController
// 鼠标: pitch/yaw 累积 → Rotate
// 键盘: WASD → Translate

// 第4步: 实现 FrustumCulling
// 从 ViewProjection 矩阵提取 6 个平面
// AABB 裁剪测试
```
# PrismaEngine 引擎开发教学手册 Part 4
## 场景、实体与游戏系统

---

## 第19章 场景图与节点系统

### 19.1 设计理念

> **"场景是世界的容器——游戏中的一切都在场景中"**

场景系统是实体和组件的运行时容器。PrismaEngine 使用 **节点树（场景图）** 模式:
- **Node**: 轻量级句柄，标识场景中的一个对象
- **Component**: 挂载在 Node 上的功能模块（Transform, Camera, MeshRenderer, ...）
- **Scene**: 管理 Node 和 Component 的容器，提供层级关系和查询

### 19.2 Scene 类

```cpp
class Scene {
public:
    Scene();
    ~Scene();

    // Node 管理
    Node CreateNode(const std::string& name = "Node");
    void RemoveNode(Node node);
    void Update(Timestep ts);
    
    // 层级 API
    void SetParent(Node child, Node parent);
    std::vector<Node> GetChildren(Node node) const;
    Node GetParent(Node node) const;
    std::vector<Node> GetRootNodes() const;
    Matrix4x4 GetWorldTransform(Node node) const;
    
    // 组件 API
    template<typename T, typename... Args>
    std::shared_ptr<T> AddComponent(Node node, Args&&... args);
    
    template<typename T>
    std::shared_ptr<T> GetComponent(Node node) const;
    
    void RemoveComponent(Node node, Component* comp);
    
    // 场景主相机
    std::shared_ptr<ICamera> GetMainCamera();
    
    // 获取场景光源
    std::vector<Light> GetLights() const;
    
    // 序列化
    bool Deserialize(const std::string& path);
    bool Serialize(const std::string& path) const;

private:
    std::string m_Name = "Untitled";
    bool m_IsDirty = false;
    std::vector<Node> m_nodes;
    std::vector<SceneNodeData> m_nodeData;
    std::vector<std::string> m_nodeNames;
    std::unordered_map<uint32_t, std::vector<std::shared_ptr<Component>>> m_nodeComponents;
};
```

### 19.3 Node 层级数据

```cpp
struct SceneNodeData {
    uint32_t parent = UINT32_MAX;          // UINT32_MAX = 根节点
    std::vector<uint32_t> children;
};
```

### 19.4 SceneManager

```cpp
class SceneManager {
public:
    Scene* CreateScene(const std::string& name);
    void DestroyScene(Scene* scene);
    
    Scene* GetActiveScene() const;
    void SetActiveScene(Scene* scene);
    
    Scene* LoadScene(const std::string& path);
    void SaveScene(Scene* scene, const std::string& path);
    
private:
    std::unordered_map<std::string, std::unique_ptr<Scene>> m_Scenes;
    Scene* m_ActiveScene = nullptr;
};
```

### 19.5 实现步骤指南

```cpp
// 第1步: 定义 SceneNodeData (父子关系)
// 第2步: 实现 CreateNode (分配 ID，初始化数据)
// 第3步: 实现组件挂钩 (unordered_map<uint32_t, vector<shared_ptr<Component>>>)
// 第4步: 实现 SceneManager (多场景管理)
```

---

## 第20章 组件系统

### 20.1 设计理念

> **"组合优于继承——用组件组装游戏对象"**

ECS 模式的核心是组合（Composition）。PrismaEngine 使用**混合架构**:
- 传统 ECS 风格的 `Node + Component` 组合
- SoA 风格的 `EntityManager` 用于高性能数据

### 20.2 Component 基类

```cpp
class Component {
public:
    Component();
    virtual ~Component();
    
    virtual void Initialize() {}    // 初始化
    virtual void Update(Timestep ts) {}  // 每帧更新
    
    Node GetOwnerNode() const { return m_OwnerNode; }
    Scene* GetScene() const { return m_Scene; }
    void SetOwnerNode(Node node, Scene* scene);
    
    virtual ComponentId GetComponentId() const = 0;
    bool IsEnabled() const { return m_Enabled; }
    void SetEnabled(bool enabled) { m_Enabled = enabled; }
    
protected:
    Node m_OwnerNode;
    Scene* m_Scene = nullptr;
    bool m_Enabled = true;
};
```

### 20.3 组件标识

使用模板技巧实现唯一的组件 ID:

```cpp
using ComponentId = uint32_t;

template<typename T>
ComponentId GetComponentTypeId() {
    static ComponentId id = s_NextComponentId++;
    return id;
}

// 用法:
// class Camera : public Component {
//     ComponentId GetComponentId() const override { 
//         return GetComponentTypeId<Camera>(); 
//     }
// };
```

### 20.4 内置组件

| 组件 | 文件 | 功能 |
|------|------|------|
| Transform | `transform/Transform.h` | 位置/旋转/缩放 |
| Camera | `transform/Camera.h` | 相机投影 |
| MeshRenderer | `graphic/MeshRenderer.h` | 网格渲染 |
| LightComponent | `graphic/LightComponent.h` | 光源 |
| SpriteRenderer | `graphic/SpriteRenderer.h` | 2D精灵 |
| RigidBody | `physics/RigidBody.h` | 物理刚体 |
| AudioSourceComponent | `audio/components/AudioSourceComponent.h` | 音频源 |
| ScriptComponent | `serialization/ScriptComponent.h` | C#脚本 |
| AIComponent | `ai/AIComponent.h` | AI状态 |
| NavAgentComponent | `navigation/NavAgentComponent.h` | 导航代理 |
| UIComponent | `ui/UIComponent.h` | UI元素 |
| ParticleEmitterComponent | `particles/ParticleEmitterComponent.h` | 粒子发射器 |

---

## 第21章 SoA 实体池与双缓冲

### 21.1 设计理念

> **"C# 不能直接访问 C++ 的对象，但可以访问 C++ 的数组"**

为了实现 C++/C# 之间的高性能数据共享，PrismaEngine 设计了一个 **SoA (Structure of Arrays)** 实体池:

- **结构体数组 (SoA)**: 同类属性连续存储，缓存友好
- **虚拟地址空间**: 1M 实体预保留，按需提交
- **双缓冲**: C++ 写 → C# 读，无锁同步

### 21.2 EntityManager

```cpp
class EntityManager {
public:
    static EntityManager& Get();
    
    Node CreateNode();
    void DestroyNode(uint32_t handle);
    
    // 双缓冲管理
    void SwapBuffers();
    TransformDataLayout* GetTransformRead();
    TransformDataLayout* GetTransformWrite();
    RenderDataLayout* GetRenderData() { return &m_layoutR; }
    
    uint32_t GetAliveCount() const;
    uint32_t GetCommittedCount() const;

private:
    void* m_blockABase;  // 缓冲 A (写)
    void* m_blockBBase;  // 缓冲 B (读)
    void* m_blockRBase;  // 渲染数据 (单缓冲)
    
    uint32_t m_aliveCount = 0;
    uint32_t m_committed = 0;
    
    int m_writeIndex = 0;  // 0: Write→A, Read→B
    std::mutex m_mutex;
};
```

### 21.3 数据布局

```cpp
// 最大虚拟实体数
static constexpr uint32_t kMaxVirtualEntities = 1024 * 1024;  // 1M
static constexpr uint32_t kCommitStep = 16384;  // 每步 16K 槽位

// Transform 数据 (SoA 布局)
struct TransformDataLayout {
    float* posX;
    float* posY;
    float* rotation;
    float* scaleX;
    float* scaleY;
};

// Render 数据 (单缓冲 SoA 布局)
struct RenderDataLayout {
    uint32_t* active;
    uint32_t* generation;
    float* colorR, * colorG, * colorB, * colorA;
    float* sizeW, * sizeH;
};
```

### 21.4 Node — 轻量级实体句柄

```cpp
struct Node {
    uint32_t handle = 0;
    
    Node() = default;
    explicit Node(uint32_t h) : handle(h) {}
    
    bool IsValid() const;
    void Destroy();
    
    uint32_t GetIndex() const { return handle & 0xFFFF; }
    uint32_t GetGeneration() const { return handle >> 16; }
    
    // Transform 属性快捷访问
    float GetX() const;
    float GetY() const;
    Vector2 GetPosition() const;
    float GetRotation() const;
    Vector2 GetScale() const;
    
    void SetX(float x);
    void SetY(float y);
    void SetPosition(const Vector2& pos);
    void SetRotation(float rot);
    void SetScale(const Vector2& scale);
};
```

### 21.5 双缓冲工作原理

```
帧 N:           帧 N+1:         帧 N+2:
C++ 写入 A       C++ 写入 B       C++ 写入 A
C# 读取 B        C# 读取 A        C# 读取 B
```

C# 每帧通过 `GetTransformRead()` 获取只读指针。
C++ 写入 `GetTransformWrite()`，然后在 `SwapBuffers()` 后使其对 C# 可见。

### 21.6 实现步骤指南

```cpp
// 第1步: 使用 mmap/VirtualAlloc 预留 1M 实体的地址空间
// Windows: VirtualAlloc(MEM_RESERVE)
// Linux: mmap(PROT_NONE)
// 按需通过 VirtualAlloc(PAGE_COMMIT) / mprotect 提交

// 第2步: 实现 SoA 布局
// 每个属性是独立的 float/u32 数组

// 第3步: 实现双缓冲
// 两个 TransformDataLayout（A 和 B）
// SwapBuffers 时交换读写角色

// 第4步: Node 句柄编码
// 高 16 位 = generation (防悬挂指针)
// 低 16 位 = index (数组索引)
```

---

## 第22章 输入系统

### 22.1 设计理念

> **"统一输入抽象，隔离平台差异"**

输入系统接收来自不同源的输入事件，统一为内部格式:
- 键盘 (KeyCode)
- 鼠标 (位置, 按钮, 滚轮)
- 游戏手柄 (轴, 按钮)
- 触摸屏 (多点触控)

### 22.2 InputManager

```cpp
namespace Input {

class InputManager {
public:
    int Initialize();
    void Shutdown();
    void Update(Timestep ts);
    
    // 键盘状态
    static bool IsKeyDown(KeyCode key);
    static bool IsKeyPressed(KeyCode key);  // 刚按下
    static bool IsKeyReleased(KeyCode key);  // 刚释放
    
    // 鼠标状态
    static bool IsMouseButtonDown(MouseButton button);
    static bool IsMouseButtonPressed(MouseButton button);
    static Vector2 GetMousePosition();
    static Vector2 GetMouseDelta();
    static float GetScrollDelta();
    
    // 游戏手柄
    static bool IsGamepadConnected(int gamepad = 0);
    static float GetAxis(GamepadAxis axis, int gamepad = 0);
    
    // 输入映射 (将物理键映射到逻辑动作)
    void BindAction(const std::string& name, KeyCode key);
    bool GetAction(const std::string& name) const;
    
private:
    std::unique_ptr<IInputDriver> m_Driver;
    // 输入状态缓存
    KeyState m_KeyStates[256];
    MouseState m_MouseState;
    std::vector<GamepadState> m_GamepadStates;
};

} // namespace Input
```

### 22.3 输入驱动抽象

```cpp
class IInputDriver {
public:
    virtual ~IInputDriver() = default;
    virtual bool Initialize() = 0;
    virtual void Update() = 0;
    virtual void Shutdown() = 0;
};

// SDL3 输入驱动
class InputDriverSDL3 : public IInputDriver {
    bool Initialize() override;
    void Update() override;  // 轮询 SDL 事件，更新 InputManager 状态
    void Shutdown() override;
};

// Win32 输入驱动 (原生)
class InputDriverWin32 : public IInputDriver {
    // 通过 Windows 消息处理输入
};
```

### 22.4 KeyCode 枚举

```cpp
enum class KeyCode {
    Unknown = 0,
    A, B, C, ..., Z,
    D0, D1, ..., D9,
    F1, F2, ..., F12,
    Space, Enter, Escape, Tab,
    LeftShift, RightShift,
    LeftControl, RightControl,
    LeftAlt, RightAlt,
    Left, Right, Up, Down,
    // ... 100+ 键码
};
```

### 22.5 EnhancedInputManager

`src/engine/input/EnhancedInputManager.h` 提供更高级的输入功能:
- 鼠标模式切换 (锁定/隐藏/捕获)
- 输入设备热插拔检测
- 游戏手柄震动控制
- 输入组合 (Chord)

```cpp
class EnhancedInputManager {
    void SetMouseMode(MouseMode mode);  // Locked, Hidden, Normal
    MouseMode GetMouseMode() const;
    
    void SetGamepadVibration(int gamepad, float left, float right);
};
```

### 22.6 实现步骤指南

```cpp
// 第1步: 定义 KeyCode 枚举和输入状态
struct KeyState {
    bool down = false;
    bool pressed = false;  // 刚按下（单帧 true）
    bool released = false; // 刚释放（单帧 true）
};

// 第2步: 实现输入驱动 (平台相关)
// SDL3: 处理 SDL_KEYDOWN/SDL_KEYUP/SDL_MOUSEMOTION
// Win32: 处理 WM_KEYDOWN/WM_KEYUP/WM_MOUSEMOVE

// 第3步: 实现 InputManager
// Update: 清除 pressed/released 状态帧标记
//         让驱动填充最新的 down 状态
//         从 down 状态计算 pressed = !oldDown && down

// 第4步: 输入映射
// string → KeyCode 映射
// 用户通过 BindAction("Jump", KeyCode::Space) 定义
```

---

## 第23章 物理与碰撞系统

### 23.1 设计理念

> **"游戏物理不需要物理引擎的精度，但需要它的速度"**

PrismaEngine 的物理系统是**自研的轻量级实现**，而非集成 PhysX 或 Box2D:
1. 足够简单: 对多数游戏来说足够
2. 完全控制: 调试/序列化/确定性
3. 无外部依赖: 跨平台零成本

### 23.2 系统架构

```
physics/
├── CollisionSystem.h    # AABB 碰撞检测
├── RigidBody.h/cpp      # 刚体定义
├── Constraint.h         # 约束接口
├── ConstraintSolver.h   # 约束求解器
├── PhysicsComponents.h  # 物理组件
├── PhysicsSystem.h/cpp  # 物理系统 (ISubSystem)
├── TriggerManager.h/cpp # 触发体积管理
├── TriggerVolume.h      # 触发体积
└── CCDSolver.h          # 连续碰撞检测
```

### 23.3 PhysicsSystem

```cpp
class PhysicsSystem : public ISubSystem {
public:
    int Initialize() override;
    void Shutdown() override;
    void Update(Timestep ts) override;

    // 刚体管理
    RigidBody* CreateRigidBody(RigidBodyType type);
    void DestroyRigidBody(RigidBody* body);
    
    // 约束管理
    void AddConstraint(IConstraint* constraint);
    void RemoveConstraint(IConstraint* constraint);
    
    // 触发体积
    uint32_t AddTrigger(const TriggerVolume& trigger);
    void RemoveTrigger(uint32_t triggerId);
    
    // 配置
    void SetGravity(const glm::dvec3& gravity);
    void SetSolverIterations(int iterations);
    void SetCCDEnabled(bool enabled);

private:
    // 物理步进
    void StepApplyForces(double dt);
    void StepApplyDamping(double dt);
    void StepIntegrateVelocity(double dt);
    void StepCCD(double dt);
    void StepCollide();
    void StepSolveConstraints(double dt);
    void StepIntegratePosition(double dt);
    void StepTriggers(double dt);

    std::vector<std::unique_ptr<RigidBody>> m_bodies;
    std::vector<IConstraint*> m_constraints;
    ConstraintSolver m_solver;
    CCDSolver m_ccdSolver;
    TriggerManager m_triggerManager;
    glm::dvec3 m_gravity{0.0, -9.81, 0.0};
    WorkerThread m_workerThread;
};
```

### 23.4 物理步进 (Physics Step)

```
PhysicsSystem::Update(dt)
    │
    ├── 1. StepApplyForces(dt)     — 应用重力和用户力
    ├── 2. StepApplyDamping(dt)    — 速度阻尼
    ├── 3. StepIntegrateVelocity   — 半隐式欧拉速度积分
    ├── 4. StepCCD(dt)             — 连续碰撞检测 (用于高速物体)
    ├── 5. StepCollide()           — 碰撞检测 → 生成接触对
    ├── 6. StepSolveConstraints    — 迭代求解约束 (N 次迭代)
    ├── 7. StepIntegratePosition   — 更新位置
    └── 8. StepTriggers(dt)        — 触发体积事件
```

### 23.5 CollisionSystem

```cpp
class CollisionSystem {
public:
    // AABB vs AABB
    static bool CheckAABB(const AABB& a, const AABB& b);
    
    // 射线 vs AABB
    static bool RayCastAABB(const Ray& ray, const AABB& aabb, RaycastHit& hit);
    
    // 扫描测试 (移动 AABB 检测首次碰撞)
    static bool SweepAABB(const AABB& source, const Vector3& velocity,
                          const AABB& target, SweepHit& hit);
    
    // 碰撞响应 (位置修正 + 速度修正)
    static void ResolveCollisions(const AABB& a, RigidBody& bodyA,
                                   const AABB& b, RigidBody& bodyB);
};
```

### 23.6 RigidBody

```cpp
class RigidBody {
public:
    RigidBodyType GetType() const;  // Static, Dynamic, Kinematic
    
    void SetMass(float mass);
    void SetLinearVelocity(const glm::dvec3& vel);
    void SetAngularVelocity(const glm::dvec3& vel);
    void ApplyForce(const glm::dvec3& force);
    void ApplyImpulse(const glm::dvec3& impulse);
    
    // 碰撞形状
    void SetAABB(const AABB& aabb);
    AABB GetAABB() const;
    
    // 物理材质
    void SetRestitution(float restitution);  // 弹性系数
    void SetFriction(float friction);         // 摩擦系数
};
```

### 23.7 实现步骤指南

```cpp
// 第1步: 实现 AABB 碰撞检测
// 重叠测试: a.max.x > b.min.x && a.min.x < b.max.x ...
// 分离轴定理的简化版本

// 第2步: 实现 RigidBody
// 半隐式欧拉积分 (速度 → 位置)
// C = -(2*0.5)*sqrt(4*0.5) - 阻尼

// 第3步: 实现碰撞对生成
// 暴力 O(n²) 或空间分区

// 第4步: 实现约束求解器
// 顺序冲量法 (Sequential Impulse)
// W. 迭代接触点约束

// 第5步: 实现 CCD
// 保守推进 (Conservative Advancement)
// 或简单的子步进 (Sub-step)
```

---

## 第24章 动画系统

### 24.1 核心类

```cpp
namespace Animation {

class AnimationSystem : public ISubSystem {
    void Update(Timestep ts) override;
    // 更新所有活跃的动画组件
};

// 动画片段
struct AnimationClip {
    struct Keyframe {
        float time;
        Vector3 position;
        Quaternion rotation;
        Vector3 scale;
    };
    
    std::vector<Keyframe> keyframes;
    float duration;
    bool looping;
    
    Transform Sample(float time) const;  // 插值
};

// 动画组件
class AnimComponent : public Component {
    std::vector<AnimationClip> clips;
    int currentClip;
    float currentTime;
    float speed = 1.0f;
    
    void Play(const std::string& name);
    void Stop();
    void Pause();
};

// 骨架
struct Skeleton {
    struct Bone {
        std::string name;
        int parent;
        Matrix4x4 bindPoseInverse;
    };
    std::vector<Bone> bones;
    std::vector<Matrix4x4> boneMatrices;  // 最终矩阵数组
};

// 状态机
class AnimStateMachine {
    struct State {
        std::string name;
        AnimationClip* clip;
        float speed;
    };
    
    struct Transition {
        int fromState;
        int toState;
        std::function<bool()> condition;
        float transitionDuration;
    };
    
    int m_CurrentState;
    std::vector<State> m_States;
    std::vector<Transition> m_Transitions;
};

} // namespace Animation
```

---

## 第25章 音频系统

### 25.1 系统架构

```
audio/
├── IAudioDevice.h         # 音频设备接口
├── AudioDevice.h/cpp      # 默认音频设备
├── AudioDeviceSDL3.h/cpp  # SDL3 后端
├── AudioDeviceNull.h/cpp  # 空实现
├── AudioAPI.h/cpp         # 高层 API
├── AudioTypes.h           # 类型定义
├── backends/              # 音频后端 (miniaudio)
├── codecs/                # 解码器 (WAV, OGG, MP3, FLAC)
├── components/            # 音频组件 (AudioSource, AudioListener)
├── core/                  # 核心接口
├── dsp/                   # 数字信号处理
│   ├── AudioNode.h        # DSP 节点图
│   ├── AudioBuffer.h      # 音频缓冲区
│   └── AudioMath.h        # DSP 数学
├── raytracing/            # 声学光线追踪
│   ├── AcousticEngine.h   # 声学引擎 (3D 空间音频)
│   └── AcousticRay.h      # 声音射线
└── SoundSynthesizer.h/cpp # 声音合成器
```

### 25.2 核心接口

```cpp
namespace Audio {

class IAudioDevice {
public:
    virtual bool Initialize(const AudioDesc& desc) = 0;
    virtual void Shutdown() = 0;
    virtual void Update() = 0;
    
    virtual AudioSource* CreateSource() = 0;
    virtual void DestroySource(AudioSource* source) = 0;
    
    virtual AudioBuffer* LoadBuffer(const std::string& path) = 0;
    virtual void PlaySource(AudioSource* source) = 0;
    virtual void StopSource(AudioSource* source) = 0;
};

} // namespace Audio
```

### 25.3 3D 空间音频

PrismaEngine 提供完整的 3D 音频:
- **AudioListenerComponent**: 场景中的听者
- **AudioSourceComponent**: 声音源 (位置、音量、滚降)
- **ReverbZoneComponent**: 混响区域
- **AcousticEngine**: 射线追踪声学模拟（遮挡/衍射/反射）

---

## 第26章 粒子系统

### 26.1 架构

```cpp
namespace Particles {

class ParticleSystem : public ISubSystem {
    void Update(Timestep ts) override;
    // 更新所有粒子发射器
    
    void Render(RenderCommandContext* ctx);
    // 渲染所有粒子 (GPU 或 CPU)
};

struct Particle {
    Vector3 position, velocity;
    Color color;
    float lifetime, maxLifetime;
    float size;
    float rotation;
};

// CPU 粒子系统 (通用)
class CPUParticleSystem {
    std::vector<Particle> particles;
    
    void Emit(const ParticleEmitterConfig& config, uint32_t count);
    void Update(float dt);
    void Render();
};

// GPU 粒子系统 (高性能，存储到缓冲区，计算着色器更新)
class GPUParticleSystem {
    // Particle data stored in VkBuffer
    // Compute shader updates particles
    // Vertex shader renders as point sprites/quads
};

} // namespace Particles
```

---

## 第27章 AI 与导航系统

### 27.1 AI 系统

```cpp
namespace AI {

class AISystem : public ISubSystem {
    void Update(Timestep ts) override;
    // 更新所有 AI 组件
};

// 有限状态机
class FiniteStateMachine {
    std::string currentState;
    std::unordered_map<std::string, StateDefinition> states;
    
    void AddState(const std::string& name, std::function<void()> update);
    void Transition(const std::string& to);
};

// 行为树
class BehaviorTree {
    // 节点类型: Sequence, Selector, Action, Condition, Decorator
    std::unique_ptr<Node> root;
    
    BehaviorStatus Tick(float dt);
};

} // namespace AI
```

### 27.2 导航系统

```cpp
namespace Navigation {

class NavigationSystem : public ISubSystem {
    void Update(Timestep ts) override;
    
    // 导航网格生成
    void BuildNavMesh(const Scene& scene);
    
    // 寻路
    std::vector<Vector3> FindPath(const Vector3& start, const Vector3& end);
    
    // 路径平滑
    std::vector<Vector3> SmoothPath(const std::vector<Vector3>& path);
};

// 导航代理组件 (挂载在游戏对象上)
class NavAgentComponent : public Component {
    std::vector<Vector3> path;
    int currentPathIndex;
    float speed;
    float stoppingDistance;
    
    void SetDestination(const Vector3& destination);
    void UpdateMovement(float dt);
};

} // namespace Navigation
```

---

## 第28章 地形系统

### 28.1 架构

```cpp
namespace Terrain {

class TerrainSystem : public ISubSystem {
    void Initialize() override;
    void Update(Timestep ts) override;
    
    // 高度图管理
    bool LoadHeightMap(const std::string& path, uint32_t width, uint32_t height, float scale);
    bool LoadHeightMapPNG(const std::string& path, float scale);
    void GenerateTestTerrain(uint32_t width, uint32_t height, float freq, float amp);
    
    // 渲染
    void SetCameraPosition(const Vector3& pos);
    void UpdateLOD(const Vector3& camera);
    void SubmitDrawCalls();
    
    // 碰撞
    float GetHeightAt(float x, float z) const;  // 高度查询
    TerrainCollision& GetCollision();
};

// 高度图
class HeightMap {
    std::vector<float> data;
    uint32_t width, height;
    float scale;
    
    float GetHeight(uint32_t x, uint32_t z) const;
    float Sample(float x, float z) const;  // 双线性插值
    void GeneratePerlin(float frequency, float amplitude, int octaves);
};

// 地形网格 (LOD)
class TerrainMesh {
    // 多 LOD 层级的网格数据
    std::vector<MeshData> lodLevels;
    
    void Generate(const HeightMap& map, const TerrainConfig& config);
    MeshData* GetLODLevel(float distance) const;
};

} // namespace Terrain
```

---

## 第29章 水体系统

### 29.1 架构

```cpp
namespace Water {

class WaterSystem : public ISubSystem {
    void Initialize() override;
    void Update(Timestep ts) override;
    
    // 波模拟
    WaveSimulation& GetWaveSimulation();
    float GetWaveHeight(const Vector2& worldPos) const;
    Vector3 GetWaveDisplacement(const Vector2& worldPos) const;
    
    // 交互 (涟漪)
    void SpawnRipple(const Vector3& position, float strength = 1.0f);
    
    // 渲染
    void CaptureReflection();  // 反射捕捉
    void CaptureRefraction();  // 折射捕捉
    void SubmitDrawCalls();
};

// 波模拟
class WaveSimulation {
    // Gerstner 波叠加
    struct Wave {
        Vector2 direction;
        float amplitude;
        float frequency;
        float speed;
        float steepness;
    };
    
    std::vector<Wave> waves;
    float time = 0.0f;
    
    Vector3 Evaluate(const Vector2& pos, float time) const;  // 位移
};

// 水面网格
class WaterMesh {
    // 细分的平面网格
    // 支持 LOD
    void Generate(const WaterConfig& config);
};

} // namespace Water
```
# PrismaEngine 引擎开发教学手册 Part 5
## 扩展系统与编辑器

---

## 第30章 网络系统

### 30.1 架构

```cpp
namespace Network {

class NetworkSystem : public ISubSystem {
public:
    int Initialize() override;
    void Update(Timestep ts) override;
    
    // 会话管理
    bool StartServer(const SessionConfig& config);
    bool StartClient(const SessionConfig& config);
    void StopSession();
    bool HasActiveSession() const;
    
    // 实体状态同步
    void RegisterScene(Scene* scene);
    NetworkId AssignNetworkId(uint32_t entityHandle);
    uint32_t FindEntity(NetworkId netId) const;
    
    // RPC (远程过程调用)
    void RegisterRPC(const std::string& name, RPCCallback callback);
    bool CallRPC(ConnectionHandle to, const std::string& name, const std::vector<uint8_t>& args = {});

private:
    void SendEntityStates();
    void ReceiveEntityStates();
    
    std::unique_ptr<Session> m_session;
    Scene* m_scene = nullptr;
    float m_tickRate = 20.0f;  // 20 次/秒状态同步
};

// 会话
class Session {
    // TCP (可靠) + UDP (不可靠) 双通道
    std::unique_ptr<ITransport> m_tcpTransport;
    std::unique_ptr<ITransport> m_udpTransport;
    
    bool Start(const SessionConfig& config);
    void Stop();
    bool IsRunning() const;
    
    void Send(Packet* packet, TransportType type);
    Packet* Receive();
};

} // namespace Network
```

### 30.2 传输层

```cpp
class ITransport {
    virtual bool Initialize(const std::string& address, uint16_t port) = 0;
    virtual void Shutdown() = 0;
    virtual bool Send(const uint8_t* data, size_t size, const Address& to) = 0;
    virtual int Receive(uint8_t* buffer, size_t maxSize, Address& from) = 0;
};

class TCPTransport : public ITransport { /* ... */ };
class UDPTransport : public ITransport { /* ... */ };
```

---

## 第31章 本地化系统

### 31.1 架构

```cpp
namespace Localization {

class LocalizationSystem : public ISubSystem {
    void SetLanguage(const std::string& code);  // "zh-CN", "en-US"
    std::string GetString(const std::string& key) const;
    
    // 从 JSON 文件加载本地化数据
    bool LoadLocalizationFile(const std::string& path);
    
private:
    std::string m_CurrentLanguage = "en-US";
    std::unordered_map<std::string, std::string> m_Strings;  // key → localized
};

} // namespace Localization
```

### 31.2 数据格式

```json
{
    "en-US": {
        "menu.play": "Play",
        "menu.settings": "Settings",
        "menu.quit": "Quit"
    },
    "zh-CN": {
        "menu.play": "开始游戏",
        "menu.settings": "设置",
        "menu.quit": "退出"
    }
}
```

---

## 第32章 控制台与 CVar 系统

### 32.1 设计理念

> **"运行时的开发者工具——无需重启就能调整引擎"**

控制台提供运行时命令行界面:
- **CVar**: 控制台变量 (可在运行时读取/修改)
- **CCommand**: 控制台命令 (执行函数)
- **自动补全**: Tab 键触发
- **历史记录**: 上下键翻

### 32.2 CVar

```cpp
class CVar {
public:
    enum Flags {
        None        = 0,
        Cheat       = 1 << 0,  // 仅在开发者模式可用
        ReadOnly    = 1 << 1,
        Save        = 1 << 2,  // 保存到配置文件
        Replicated  = 1 << 3,  // 网络同步
    };
    
    const std::string& GetName() const;
    std::string GetString() const;
    void SetString(const std::string& value);
    
    template<typename T>
    T Get() const;  // 类型安全访问
    
private:
    std::string m_Name;
    std::string m_Value;
    std::string m_DefaultValue;
    Flags m_Flags;
    std::string m_Description;
};

// 宏简化定义
#define CVAR(name, defaultValue, flags, description) \
    static CVar s_##name(#name, defaultValue, flags, description)

// 使用示例
CVAR(r_renderScale, "1.0", CVar::Save, "渲染分辨率缩放");
CVAR(r_maxFPS, "144", CVar::Save, "最大 FPS");
CVAR(cl_showFPS, "0", CVar::None, "显示 FPS 计数器");
```

### 32.3 控制台命令

```cpp
class ConsoleCommand {
    std::string m_Name;
    std::string m_Help;
    std::function<void(const std::vector<std::string>&)> m_Action;
};

// 使用
ConsoleSystem::RegisterCommand("help", "显示帮助信息", [](auto&) {
    // 列出所有命令
});
```

### 32.4 ConsoleSystem

```cpp
class ConsoleSystem : public ISubSystem {
    void Execute(const std::string& command);
    
    void RegisterCVar(CVar* cvar);
    void RegisterCommand(const std::string& name, const std::string& help,
                         std::function<void(const std::vector<std::string>&)> action);
    
    CVar* FindCVar(const std::string& name);
    
    // 保存/加载 CVar (到 jsonc 文件)
    void SaveCVars(const std::string& path);
    void LoadCVars(const std::string& path);
};
```

### 32.5 ConsoleUI — ImGui 终端

```cpp
class ConsoleUI {
    void OnImGuiRender();  // ImGui 窗口
    
    std::vector<std::string> m_History;   // 命令历史
    std::vector<LogEntry> m_LogBuffer;    // 输出缓冲区
    char m_InputBuffer[256];              // 输入缓冲区
    int m_HistoryIndex = -1;
};
```

---

## 第33章 脚本系统 (CoreCLR & Mono)

### 33.1 设计理念

> **"用 C# 写游戏逻辑，用 C++ 运行它"**

PrismaEngine 支持两种脚本后端:
1. **CoreCLR (.NET 10+)**: 主要后端，self-contained 发布
2. **Mono (向后兼容)**: 传统 Mono 运行时

### 33.2 CoreCLRHost

```cpp
namespace Scripting {

class CoreCLRHost {
public:
    bool Initialize(const std::string& scriptsDir);
    void Shutdown();
    
    // 获取 C# [UnmanagedCallersOnly] 方法的函数指针
    void* GetFunctionPointer(const std::string& assemblyPath,
                              const std::string& typeName,
                              const std::string& methodName);

private:
    void* m_hostfxrLib = nullptr;      // hostfxr 动态库句柄
    void* m_hostContext = nullptr;      // CoreCLR 宿主上下文
    bool m_initialized = false;
    std::string m_scriptsDir;
};
```

**CoreCLR 初始化流程**:
```
1. 从 scriptsDir 加载 hostfxr.dll/so
2. 初始化 CoreCLR 运行时
3. 加载目标 Assembly (.dll)
4. 通过 [UnmanagedCallersOnly] 获取入口函数指针
5. C++ 直接调用函数指针 → C# 代码执行
```

### 33.3 ScriptEngine

```cpp
class ScriptEngine {
    void Initialize();
    void Update(Timestep ts);  // 每帧调 C# 的 OnUpdate()
    void Shutdown();
    
    // 在 C# 中执行代码
    void ExecuteScript(const std::string& source);
    
    // 获取/设置 C# 脚本中的变量
    void SetVariable(const std::string& name, void* value);
    void* GetVariable(const std::string& name);
};

// C# 侧结构
// Prisma.Core/ 包含:
// - Node.cs (C# 端的 Node 句柄)
// - WorldManager.cs (场景管理)
// - ScriptEntry.cs (入口点: Bootstrap, OnFrame)
// - SRPGraphicsAPI.cs (可编程渲染管线)
```

### 33.4 C# → C++ 互操作

```
C# [UnmanagedCallersOnly] → C++ 函数指针
C# 通过 InternalsVisibleTo 访问引擎 API
数据通过 SoA 实体池共享 (共享内存指针)

C# 构件: Prisma.Bindings.dll + Prisma.Generators.dll + 游戏 Assembly (.Scripts.dll)
C++ Engine 加载 CoreCLR → Bootstrap() → OnFrame() 循环
```

### 33.5 实现步骤指南

```cpp
// 第1步: 检测/加载 CoreCLR hostfxr
// 从 self-contained publish 目录加载

// 第2步: 初始化运行时
// hostfxr_initialize_for_runtime_config
// hostfxr_get_runtime_delegate → load_assembly_and_get_function_pointer

// 第3步: 加载 C# 程序集
// 获取 Bootstrap 和 OnFrame 函数指针

// 第4步: 每帧调用
// Engine::Update 中调用 scriptEngine->Update(ts)
```

---

## 第34章 编辑器架构

### 34.1 设计理念

> **"编辑器是引擎的窗口——所见即所得的开发体验"**

PrismaEditor 是一个独立的可执行文件，链接 PrismaEngine:
```
PrismaEditor.exe
├── src/editor/
│   ├── core/           # 编辑器核心
│   │   ├── Editor.h/cpp      # 编辑器主类
│   │   ├── EditorService.h   # 编辑器服务
│   │   ├── WebUIEditor.h/cpp # WebUI 后端
│   │   └── CommandLineEditor.h/cpp
│   ├── panels/         # 编辑器面板
│   │   ├── EditorLayer.h/cpp  # ImGui 面板层
│   │   └── ProfilerPanel.h/cpp
│   ├── windows/        # 编辑器窗口
│   │   └── ProjectSettingsWindow.h/cpp
│   ├── graphic/        # 编辑器图形
│   │   ├── ViewportRenderPass.h/cpp  # 编辑器视口
│   │   └── ImGuiVulkanResourceManager.h/cpp
│   └── mcp/            # MCP 服务器
│       ├── MCPServer.h/cpp
│       ├── MCPSession.h/cpp
│       ├── MCPTool.h/cpp
│       ├── transport/   # MCP 传输层
│       ├── tools/       # MCP 工具定义
│       └── session/     
├── 链接 PrismaEngine (Engine 库)
```

### 34.2 编辑器面板

```cpp
// EditorLayer — ImGui 面板系统
class EditorLayer : public Layer {
    void OnImGuiRender() override {
        // 菜单栏
        // 场景视图 (Viewport)
        // 层级面板 (Hierarchy)
        // 属性面板 (Inspector)
        // 资源面板 (Asset Browser)
        // 控制台 (Console)
    }
};
```

### 34.3 编辑器核心结构

```
┌─────────────────────────────────────────────────────┐
│ 菜单栏 (File, Edit, View, Help)                      │
├────────────┬──────────────────────────┬──────────────┤
│ 层级面板     │                          │ 属性面板      │
│ Hierarchy  │   场景视口 (Viewport)     │ Inspector    │
│            │                          │              │
│ ─ Scene    │   [3D 视图 - 游戏渲染]    │ Transform    │
│   ├─ Camera│                          │   Pos 0,0,0 │
│   ├─ Light │                          │   Rot 0,0,0 │
│   └─ Cube  │                          │   Scl 1,1,1 │
│            │                          │              │
├────────────┼──────────────────────────┴──────────────┤
│ 资源浏览器   │  控制台 (Console)                       │
│ Assets     │  > help                                   │
└────────────┴──────────────────────────────────────────┘
```

---

## 第35章 MCP 协议与 AI 集成

### 35.1 概念

**MCP (Model Context Protocol)** 让 AI Agent 可以直接控制编辑器：
- 17+ 工具 (创建实体、修改属性、执行命令)
- 双传输通道 (STDIO + HTTP SSE)
- 哈希增量追踪 (文件修改自动同步)

### 35.2 架构

```cpp
// MCPServer — MCP 协议服务器
class MCPServer {
    void Start(TransportType type);  // STDIO / HTTP
    void Stop();
    
    // 注册工具
    void RegisterTool(const MCPTool& tool);
    
    // 处理请求
    void HandleRequest(const MCPRequest& req);
    void SendResponse(const MCPResponse& resp);
};

// MCPTool — 工具定义
class MCPTool {
    std::string name;
    std::string description;
    std::vector<Parameter> parameters;
    
    // 执行函数
    std::function<MCPResponse(const MCPRequest&)> execute;
};

// MCPSubSystem — 作为引擎子系统存在
class MCPSubSystem : public ISubSystem {
    int Initialize() override;
    void Shutdown() override;
    void Update(Timestep) override;
    
    MCPServer& GetServer();
};
```

### 35.3 工具分类 (7 类)

```
编辑器控制: 打开/保存场景, 撤销/重做
场景操作: 创建/删除/修改实体
资源管理: 导入/删除/修改资源
调试: 执行 CVar, 查看日志
渲染: 切换到不同管线, 调整渲染设置
AI 辅助: 代码生成, 场景自动布置
自定义: 通过脚本注册的自定义工具
```

### 35.4 实现步骤指南

```cpp
// 第1步: 实现 MCP 协议传输层
// STDIO: 通过 stdin/stdout 传输 JSON-RPC 消息
// HTTP SSE: 通过 HTTP POST/Server-Sent Events

// 第2步: 实现 MCPTool 注册和执行
// 每个工具是一个 name + description + handle 函数

// 第3步: 实现 MCPSubSystem
// 在引擎主循环中更新 MCP 连接
```

---

## 第36章 ImGui 集成

### 36.1 设计理念

ImGui (Dear ImGui) 是编辑器和调试工具的核心 UI 框架:
- **立即模式**: 每帧重新生成 UI 状态，无需保留 UI 对象
- **平台无关**: 渲染后端的 Vulkan 集成
- **可嵌入**: 渲染到引擎的交换链或离屏 RT

### 36.2 渲染集成

```cpp
// Editor 图形资源管理
class ImGuiVulkanResourceManager {
    bool Initialize(VkDevice device, VkDescriptorPool pool);
    void Shutdown();
    
    // ImGui 的 Vulkan 渲染初始化
    void InitImGuiVulkan(VkRenderPass renderPass);
    void RenderDrawData(ImDrawData* drawData, VkCommandBuffer cmd);
};

// 视口渲染 Pass
class ViewportRenderPass : public RenderPass {
    void Execute() override;
    // 渲染场景到编辑器视口纹理
    // 然后 ImGui 渲染覆盖层
};
```

### 36.3 ImGui 层

```cpp
class ImGuiLayer : public Layer {
    void OnAttach() override;   // Init ImGui
    void OnDetach() override;   // Shutdown ImGui
    void OnImGuiRender() override;  // 渲染 UI
    void OnEvent(Event& e) override;  // 输入事件转发到 ImGui
    
    void Begin();  // ImGui::NewFrame()
    void End();    // ImGui::Render()
};
```

---

## 第37章 WebUI 编辑器

### 37.1 设计理念

WebUI 编辑器是**浏览器内编辑器**:
- 通过 HTTP 服务提供 UI (HTML/JS/CSS)
- 通过 WebSocket 实现实时通信
- 场景视图通过 MJPEG 或 WebRTC 流式传输

### 37.2 组件

```cpp
// WebUI 编辑器后端
class WebUIEditor {
    bool Start(int port);     // 启动 HTTP 服务器
    void Stop();
    
    // 处理 WebSocket 消息
    void HandleMessage(const std::string& msg);
    
    // 推送场景状态到浏览器
    void PushSceneState(const Scene& scene);
    
private:
    httplib::Server m_HttpServer;
    int m_Port = 8080;
};
```

### 37.3 架构

```
浏览器 (HTML/CSS/JS)
    │ HTTP + WebSocket
    ▼
Editor (C++)
    ├── WebUIEditor.cpp  (HTTP 服务器)
    ├── MCPServer.cpp    (AI Agent 协议)
    └── NativeWebViewLoader.h (WebView 嵌入)
```

---

## 第38章 着色器系统与 SPIR-V 管线

### 38.1 着色器编译管线

```cpp
// 着色器加载工作流:
// GLSL/HLSL 源码 → glslangValidator/glslc → SPIR-V (.spv)
// → VkShaderModule → 绑定到 VkPipelineShaderStage

// Shader 类
class Shader {
    bool Load(const std::string& path);    // 加载 .spv 文件
    bool Compile(const std::string& source, ShaderType type);  // 运行时编译
    VkShaderModule GetNativeModule() const;
    
    // 反射信息 (通过 SPIRV-Reflect)
    struct ReflectionInfo {
        std::vector<DescriptorSetInfo> descriptorSets;
        std::vector<PushConstantRange> pushConstants;
        std::vector<VertexInputInfo> vertexInputs;
    };
    ReflectionInfo Reflect() const;
};
```

### 38.2 着色器存储

```
resources/common/shaders/
├── glsl/           # GLSL 源码
│   ├── forward.vert
│   ├── forward.frag
│   └── pbr.frag
└── hlsl/           # HLSL 源码
    ├── forward.hlsl
    └── pbr.hlsl
```

### 38.3 嵌入着色器

```cpp
// DefaultShader.h — 引擎内置的默认着色器
// 在无法加载外部文件时使用
// 编译为 C++ 头文件中的字符串常量

static const char* s_DefaultVertexShader = R"(
#version 450
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;
// ...
void main() {
    gl_Position = u_MVP * vec4(a_Position, 1.0);
}
)";
```

---

## 第39章 序列化系统

### 39.1 设计理念

PrismaEngine 使用 **glaze** JSON 库进行序列化:
- 编译时反射 (不依赖运行时类型信息)
- 高性能 (比 nlohmann/json 快 10x-50x)
- 支持二进制格式

### 39.2 序列化使用

```cpp
struct Serializable : public ISerializable {
    virtual bool Serialize(const std::string& path) const;
    virtual bool Deserialize(const std::string& path);
};

// Scene 序列化
bool Scene::Serialize(const std::string& path) const {
    // 将场景中的 Node, Component, 层级关系写入 JSON
    std::string json = glz::write_json(sceneData);
    std::ofstream file(path);
    file << json;
    return true;
}

bool Scene::Deserialize(const std::string& path) {
    std::ifstream file(path);
    std::string json((std::istreambuf_iterator<char>(file)), {});
    SceneData data;
    glz::read_json(data, json);
    // 重建场景
    return true;
}

// ProjectConfig — 项目配置文件 (project.jsonc)
// 使用 glaze 读取
```

---

## 第40章 打包与分发

### 40.1 SDK 打包

```cmake
# cmake/SDKConfig.cmake
# 收集 public headers，创建 CMake 配置模板
# 打包为 tar.gz/zip

# 构建 SDK:
cmake --build build --target sdk-package
```

### 40.2 CPack 打包

```cmake
# cmake/PackagingConfig.cmake
# Windows: NSIS 安装包 + ZIP
# Linux: TGZ + DEB + RPM
# Android: APK (通过 Gradle)

# 打包:
cmake --build build --target package
```

### 40.3 打包管线

```cpp
namespace Packing {
    class Pipeline {
        // 1. 编译着色器
        // 2. 打包资源 (纹理压缩)
        // 3. 序列化场景
        // 4. 生成最终包
        // 5. 签名/加密 (可选)
    };
}
```

---

## 第41章 跨平台与 Android

### 41.1 平台分层图

```
┌──────────────────────────────────────────────────┐
│ 游戏代码 / 应用代码                                │
├──────────────────────────────────────────────────┤
│ Engine Core (平台无关)                            │
├──────────┬──────────┬──────────┬─────────────────┤
│ Windows  │  Linux   │ Android  │ 未来平台        │
│ Backend  │ Backend  │ Backend  │ (macOS/iOS)    │
├──────────┴──────────┴──────────┴─────────────────┤
│ 平台层: Win32 API, X11/Wayland, GameActivity     │
│ 图形层: Vulkan (所有平台), DX12 (仅 Windows)     │
│ 音频层: WASAPI (Win), PulseAudio (Linux), AAudio  │
│ 输入层: Raw Input (Win), evdev (Linux)           │
└──────────────────────────────────────────────────┘
```

### 41.2 Android 深入优化

- **GameActivity**: 零延迟输入（整合 GameTextInput）
- **Vulkan**: VK_KHR_android_surface, 异步队列提交
- **性能**: DOTNET_GCRegionRange 用于 CoreCLR
- **APK**: Gradle 构建系统集成

```kotlin
// projects/android/PrismaAndroid/build.gradle.kts
android {
    ndkVersion = "28.0.12674087"
    defaultConfig {
        minSdk = 34
        targetSdk = 35
        externalNativeBuild {
            cmake {
                arguments += "-DPRISMA_BUILD_EDITOR=OFF"
                arguments += "-DPRISMA_BUILD_LAUNCHER=ON"
            }
        }
    }
}
```

### 41.3 平台检测宏

```cpp
// 在 Build.h 中定义 (由 CMake 生成)

#if defined(__ANDROID__)
    #define PRISMA_PLATFORM_ANDROID 1
#elif defined(_WIN32)
    #define PRISMA_PLATFORM_WINDOWS 1
#elif defined(__linux__)
    #define PRISMA_PLATFORM_LINUX 1
#endif

#if defined(PRISMA_PLATFORM_ANDROID) || defined(PRISMA_PLATFORM_LINUX)
    #define PRISMA_PLATFORM_UNIX 1
#endif
```

---

## 总结：从零到引擎

PrismaEngine 是一份**完整的 C++23 游戏引擎实现**，涵盖从底层构建系统到高层游戏逻辑的完整栈。

### 学习路径建议

```
1. 构建系统 + 平台层      → 理解项目如何组织
2. 数学 + 内存 + 日志     → 引擎基础
3. Engine 主循环 + Event  → 引擎心跳
4. 作业系统 + 多线程      → 性能基础
5. 渲染抽象 + Vulkan实现  → 图形核心
6. 场景 + 组件系统       → 游戏对象模型
7. RenderGraph + 管线     → 渲染架构
8. 物理 + 音频 + 输入     → 交互系统
9. 动画 + 粒子 + AI      → 高级游戏特性
10. 脚本系统 (CoreCLR)    → 可扩展性
11. 编辑器 + MCP         → 工具链
12. 打包 + 跨平台         → 发布
```

### 关键技术栈

| 类别 | 技术 |
|------|------|
| 语言 | C++23 (概念, 协程, 设计初始化器) |
| 构建 | CMake 3.31+, FetchContent |
| 图形 | Vulkan 1.3 (vk-bootstrap + VMA) |
| 数学 | GLM |
| 脚本 | CoreCLR (.NET 10), Mono |
| UI | Dear ImGui, WebUI |
| 序列化 | glaze (JSON) |
| 着色器 | GLSL/HLSL → SPIR-V |
| 协议 | MCP (AI 集成) |

**完整代码**: https://github.com/Excurs1ons/PrismaEngine
