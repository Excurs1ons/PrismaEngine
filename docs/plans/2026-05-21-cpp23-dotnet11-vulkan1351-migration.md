# C++23 / .NET 11 / Vulkan 1.4.351 迁移计划

**Status:** 📋 计划阶段  
**Date:** 2026-05-21  
**Scope:** 引擎基础工具链 + SDK 版本升级  
**Estimated Effort:** 8-12 人天（含验证）

---

## 1. 动机

当前 PrismaEngine 使用 C++20 + .NET 10 + Vulkan SDK 1.4.350。三个技术栈均已进入新的稳定版本，带来显著改进：

| 维度 | 当前 | 目标 | 核心收益 |
|------|------|------|----------|
| **C++** | C++20 | C++23 | `std::expected`(零开销错误处理)、`std::flat_map`(缓存友好容器)、`std::mdspan`(多维视图)、`std::print`(快速格式化) |
| **.NET** | 10 | 11 | Runtime Async v2(减少 GC 分配)、JIT 边界检查消除、C# 15 Union Types |
| **Vulkan** | SDK 1.4.350.0 | SDK 1.4.351.0 | VK_KHR_opacity_micromap(光线追踪)、VK_EXT_shader_split_barrier(GPU 同步)、SPIR-V 路径统一 |

---

## 2. 变更文件清单（按优先级分组）

### 2.1 P0 — 核心构建配置（必须变更，阻塞编译）

#### C++ 标准 (20 → 23)

| # | 文件 | 变更 |
|---|------|------|
| 1 | `cmake/CompilerOptions.cmake:6` | `CMAKE_CXX_STANDARD 20` → `23` |
| 2 | `projects/Template2D/CMakeLists.txt:11` | `CMAKE_CXX_STANDARD 20` → `23` |
| 3 | `projects/Template3D/CMakeLists.txt:11` | `CMAKE_CXX_STANDARD 20` → `23` |
| 4 | `projects/PrismaCraft/CMakeLists.txt:11` | `CMAKE_CXX_STANDARD 20` → `23` |
| 5 | `projects/PacManGame/CMakeLists.txt:11` | `CMAKE_CXX_STANDARD 20` → `23` |
| 6 | `sdk/samples/BasicTriangle/CMakeLists.txt:4` | `CMAKE_CXX_STANDARD 20` → `23` |
| 7 | `sdk/samples/BlockGame/CMakeLists.txt:4` | `CMAKE_CXX_STANDARD 20` → `23` |
| 8 | `sdk/samples/PrismaCraftStarter/CMakeLists.txt:4` | `CMAKE_CXX_STANDARD 20` → `23` |
| 9 | `.github/workflows/release-sdk-linux-arm64.yml:218` | 内联模板 `CMAKE_CXX_STANDARD 20` → `23` |
| 10 | `scripts/package-sdk.sh:275` | 内联模板 `CMAKE_CXX_STANDARD 20` → `23` |

#### .NET TF (10.0 → 11.0)

| # | 文件 | 变更 |
|---|------|------|
| 11 | `src/engine/scripting/CSharp/Prisma.Core/Prisma.Core.csproj:3` | `net10.0` → `net11.0` |
| 12 | `projects/Template2D/scripts/GameScripts/GameScripts.csproj:3` | `net10.0` → `net11.0` |
| 13 | `projects/PrismaCraft/scripts/GameScripts/GameScripts.csproj:3` | `net10.0` → `net11.0` |
| 14 | `src/engine/scripting/CSharp/Prisma.Generators/Prisma.Generators.csproj:4` | `netstandard2.0` → **保持不动**（Roslyn source gen） |

#### .NET 构建路径 (net10.0 → net11.0)

| # | 文件 | 变更 |
|---|------|------|
| 15 | `projects/Template3D/CMakeLists.txt:174` | `net10.0/${CS_RID}/publish` → `net11.0/...` |
| 16 | `projects/Template2D/CMakeLists.txt:170` | `net10.0/${CS_RID}/publish` → `net11.0/...` |
| 17 | `projects/PrismaCraft/CMakeLists.txt:181` | `net10.0/${CS_RID}/publish` → `net11.0/...` |
| 18 | `src/engine/app/Engine.cpp:230-232` | 3 处 `net10.0/win-x64/publish` 硬编码路径 → `net11.0/...` |

#### Vulkan 依赖版本

| # | 文件 | 变更 |
|---|------|------|
| 19 | `cmake/DependencyVersions.cmake:61` | `VULKAN_HEADERS_VERSION "v1.4.328"` → `"v1.4.351"` |
| 20 | `cmake/DependencyVersions.cmake:69` | `VK_BOOTSTRAP_VERSION "v1.4.343"` → `"v1.4.351"` |
| 21 | `cmake/DependencyVersions.cmake:73` | `SPIRV_REFLECT_VERSION "vulkan-sdk-1.4.350.0"` → `"vulkan-sdk-1.4.351.0"` |

#### Vulkan API 版本（C++ 源码）

| # | 文件 | 行 | 变更 |
|---|------|-----|------|
| 22 | `src/engine/config/RenderBackendConfig.h` | 22-23 | `VULKAN_REQUIRED_VERSION_MINOR 3` → 保持 1.3（不需要提升至 1.4 — SDK 1.4.x 向下兼容 1.3 API） |
| 23 | `src/editor/core/Editor.cpp` | 134 | `VK_API_VERSION_1_3` → 保持（同上） |
| 24 | `src/engine/graphic/adapters/vulkan/RenderDeviceVulkan.cpp` | 146 | `VK_API_VERSION_1_3` → 保持（同上） |

### 2.2 P1 — CI/CD（构建基础设施）

| # | 文件 | 变更 |
|---|------|------|
| 25 | `.github/workflows/ci-windows.yml:55` | `vulkan-query-version: 1.4.304.0` → `1.4.351.0` |
| 26 | `.github/workflows/ci-linux.yml:59` | `vulkan-query-version: 1.4.304.0` → `1.4.351.0` |
| 27 | `.github/workflows/build-windows-editor.yml:58,114` | `vulkan-query-version: 1.3.296.0` → `1.4.351.0`（**Bug fix**：当前编辑器 CI 使用 1.3.x，落后于引擎的 1.4.x） |
| 28 | `.github/workflows/ci-android.yml:47` | `ndk-version: r28` → `r29`（可选，r28 继续可用） |
| 29 | `.github/workflows/ci-android.yml:42` | `java-version: '17'` → `'21'`（可选，JDK 17 继续可用） |

### 2.3 P2 — 文档更新

| # | 文件 | 当前内容 | 新内容 |
|---|------|----------|--------|
| 30 | `README.md` | `C++20`（多处） | `C++23` |
| 31 | `CLAUDE.md` | `C++20`（多处） | `C++23` |
| 32 | `GEMINI.md` | `C++20`（多处） | `C++23` |
| 33 | `docs/CodingStyle.md:4` | `> **版本**: C++20` | `C++23` |
| 34 | `docs/MEMO.md:7` | `**编程语言**: C++20` | `C++23` |
| 35 | `docs/README_zh.md` | `现代 C++20` | `C++23` |
| 36 | `docs/Index.md` | `C++20`（如有引用） | `C++23` |
| 37 | `cmake/DEPENDENCY_VERSIONING.md` | Vulkan-Headers v1.4.328 等 | 更新为 v1.4.351 |
| 38 | `docs/ScriptingSystem.md` | `.NET 10` / `DOTNET_GCRegionRange` | `.NET 11`（Termux 说明保留） |

---

## 3. 技术分析

### 3.1 C++20 → C++23：高价值特性

#### 第一梯队（立即采用，高回报）

| 特性 | 引擎使用场景 | 收益 | 编译器要求 |
|------|-------------|------|-----------|
| **`std::expected<T,E>`** | OBJParser 错误处理、Vulkan 资源创建、文件 I/O | 零开销替代异常；Rust 风格 `and_then()`/`transform()` 链式调用 | GCC 12+, Clang 16+, MSVC 19.34+ |
| **`std::flat_map`** | 替换 32 处 `std::unordered_map`（资源缓存、着色器管理、材质注册等读多写少场景） | 连续内存 → 缓存友好，查询比 `unordered_map` 快 30-80% | GCC 13+, Clang 17+, MSVC 19.34+ |
| **`std::print` / `std::println`** | 日志系统、调试输出 | 比 `printf` 更快（类型安全），Unicode 原生支持，比 `cout` 快 2-3 倍 | GCC 13+, Clang 17+, MSVC 19.34+ |
| **`std::mdspan`** | 纹理像素数据、FrameBuffer 附件、着色器常量缓冲 | 零开销多维视图，替代手动 `y*width+x` 计算 | GCC 12+, Clang 17+, MSVC 19.31+ |

#### 第二梯队（渐进式采用）

| 特性 | 引擎使用场景 | 收益 |
|------|-------------|------|
| **`deducing this`** | 简化 `Singleton<T>` CRTP、消除 const/non-const 重载 | 一个函数模板替代 4 个重载 |
| **`std::move_only_function`** | 替换 `JobSystem` 中的 `std::function<void()>` | 移动语义，避免拷贝 |
| **`std::generator`** | 异步资源加载流程（等待 Clang 支持） | 标准库协程生成器 |
| **`if consteval`** / **`[[assume]]`** | 编译期/运行时分叉，编译器优化提示 | 更清晰的意图表达 |

#### 编译器兼容性

| 平台 | 编译器 | C++23 核心支持 |
|------|--------|----------------|
| **Termux/ARM64** (Ubuntu 25.10) | GCC 15+ | ✅ 完整 |
| **Termux/ARM64** | Clang 19+ | ✅ 完整（除 `std::generator`） |
| **Windows** | MSVC 19.51 (VS 2026) | ✅ 完整 |
| **Android NDK** | Clang 18+ (NDK r27+) | ✅ `expected`/`mdspan`/`flat_map`/`print` |
| **Linux x64 CI** | apt `g++-14` 或 `clang-18` | ✅ 完整 |

#### 破坏性变更（C++20 → C++23）

| 变更 | 影响 | 应对 |
|------|------|------|
| Simpler implicit move (P2266R3) | `return t;` 在模板中变为 xvalue | 极少场景，编译期直接报错 |
| 移除 GC 支持 (P2186R2) | 无（引擎不使用 `declare_reachable`） | 无需处理 |
| Deprecate `std::aligned_storage` | 自定义内存分配器 | 改用 `alignas` + 数组 |

---

### 3.2 .NET 10 → 11：Native Hosting 影响

#### 核心发现

| 项目 | .NET 10 → .NET 11 |
|------|-------------------|
| **HostFxr API 签名** | ✅ **无变更** — `hostfxr.h` 所有导出函数向后兼容 |
| **P/Invoke 源生成器** | ✅ 无变更 — `LibraryImport` / `UnmanagedCallersOnly` 继续工作 |
| **netstandard2.0** | ✅ 完全向后兼容 — `Prisma.Generators.csproj` 无需修改 |
| **ARM64 支持** | ✅ Oryon CPU 满足 armv8.0-a + LSE 基线要求 |

#### Runtime Async v2（头号特性）

自 C# 5 async/await 以来最重大的异步架构变更：
- 编译器不再生成状态机类 → 运行时直接管理暂停/恢复
- 堆栈追踪从 13 帧 → 5 帧
- 减少 GC 分配（更激进地复用 continuation 对象）
- 支持 NativeAOT + ReadyToRun

**对 PrismaEngine C# 脚本**：如果脚本中大量使用 async（区块加载、资源流式加载），收益显著。

启用方式（`Prisma.Core.csproj`）：
```xml
<PropertyGroup>
  <TargetFramework>net11.0</TargetFramework>
  <Features>runtime-async=on</Features>
</PropertyGroup>
```

#### JIT 优化（直接影响热循环）

| 优化 | 适用场景 |
|------|---------|
| 边界检查消除 | 区块遍历、数组操作 (`chunkSections[i]`) |
| switch 表达式折叠 | 方块状态/类型调度 |
| `SequenceEqual` 常量折叠 | 方块 ID 字符串比较 |
| `uint` → `float`/`double` 加速 | 坐标转换、物理计算 |
| Arm64 `IndexOfAnyAsciiSearcher` | 字符串/字节搜索 5-50% 改进 |

#### C# 15 Union Types

```csharp
// 类型安全的方块状态建模
public union struct BlockType { Air, Stone, Dirt, Grass, Water case Block Water; }

public record Water(int Level)
{
    public int Level { get; } = Level;
}
```

#### .NET 11 版本策略

| 属性 | 值 |
|------|-----|
| **发布类型** | STS (Standard Term Support) |
| **支持周期** | 24 个月 |
| **GA 预计** | 2026 年 11 月 |
| **当前阶段** | Preview 4 (2026-05) |
| **最低 CPU** | x86-64-v2 (POPCNT/SSE4.2); ARM64 LSE |

---

### 3.3 Vulkan SDK 1.4.350 → 1.4.351

#### 迁移风险：✅ 极低（纯头文件替换 + 重新编译）

| 项目 | 状态 |
|------|------|
| **破坏性变更** | ❌ 无 |
| **API 弃用** | ❌ 无 |
| **VMA 兼容性** | ✅ 不受影响 (v3.1.0) |
| **vk-bootstrap 兼容性** | ✅ 不受影响 |
| **头文件兼容性** | ✅ 完全向后兼容 |
| **代码修改量** | 0 行（Vulkan 部分），仅更新版本号 |

#### Spec 1.4.351 新增 6 个扩展

| 扩展 | 类型 | 对 PrismaEngine 价值 |
|------|------|---------------------|
| `VK_KHR_opacity_micromap` | 光线追踪 | ⚪ 未来（光追实现时使用） |
| `VK_EXT_shader_split_barrier` | GPU 同步 | 🟢 低（compute-heavy 场景有用） |
| `VK_AMD_gpa_interface` | AMD 专用 | ❌ 无关 |
| `VK_QCOM_elapsed_timer_query` | Qualcomm 专用 | ❌ 无关 |
| `VK_QCOM_image_processing3` | Qualcomm 专用 | ❌ 无关 |
| `VK_QCOM_shader_multiple_wait_queues` | Qualcomm 专用 | ❌ 无关 |

#### ⚠️ 唯一需注意的变更：SPIR-V 路径统一

SDK 1.4.350 已废弃 `/Include/spirv-headers`，SDK 1.4.351 将**删除**旧路径。

```cmake
# 旧路径（1.4.350 废弃警告）:
find_path(SPIRV_HEADERS_DIR Include/spirv-headers)

# 新路径（1.4.351 要求）:
find_path(SPIRV_HEADERS_DIR Include/spirv/unified1)
```

检查：`cmake/FetchThirdPartyDeps.cmake` 中是否有显式引用 `spirv-headers` 路径。

#### Ubuntu CI 要求变更

SDK 1.4.350 是最后一个支持 Ubuntu 22.04 LTS 的版本。SDK 1.4.351 要求 **Ubuntu 24.04+**。

如果 Linux CI 使用 `ubuntu-22.04` runner → 需迁移到 `ubuntu-24.04` 或 `ubuntu-latest`。

---

## 4. 实施阶段

```
Phase 1: 工具链准备          (1-2 人天)
Phase 2: 核心迁移（无特性变更） (2-3 人天)
Phase 3: 渐进式特性采用       (3-5 人天)
Phase 4: 回归测试与文档       (2 人天)
```

### Phase 1 — 工具链准备

- [ ] **1.1** 在 Termux/ARM64 环境安装 .NET 11 SDK Preview
  ```bash
  # 下载 ARM64 二进制
  curl -sLO https://dotnetcli.azureedge.net/dotnet/Sdk/11.0.100-preview.4/dotnet-sdk-11.0.100-preview.4-linux-arm64.tar.gz
  mkdir -p $HOME/dotnet && tar xzf dotnet-sdk-*-linux-arm64.tar.gz -C $HOME/dotnet
  export DOTNET_ROOT=$HOME/dotnet && export PATH=$PATH:$HOME/dotnet
  ```
- [ ] **1.2** 确认 GCC 15+ (或 Clang 19+) 可用 → `g++ --version`
- [ ] **1.3** 在 .NET 11 上做兼容性构建 → `dotnet build`
- [ ] **1.4** 升级 CI Docker 镜像到 ubuntu:noble (24.04) 或 oracular (25.04)

### Phase 2 — 核心迁移（无特性变更）

- [ ] **2.1** 更新 `cmake/DependencyVersions.cmake`（Vulkan Headers `v1.4.351`、vk-bootstrap `v1.4.351`、SPIRV-Reflect `vulkan-sdk-1.4.351.0`）
- [ ] **2.2** 更新 `cmake/CompilerOptions.cmake` → `CMAKE_CXX_STANDARD 23`
- [ ] **2.3** 更新全部 7 个 projects/samples CMakeLists.txt → `CXX_STANDARD 23`
- [ ] **2.4** 更新 4 个 .csproj → `TargetFramework net11.0`（除 Generators 保持 netstandard2.0）
- [ ] **2.5** 更新构建路径（3 CMakeLists.txt + Engine.cpp 中的 `net10.0` → `net11.0`）
- [ ] **2.6** 更新 CI/CD（9 个 workflow 文件中的 Vulkan SDK 版本 + CMake 版本统一）
- [ ] **2.7** 全平台编译验证：Windows (MSVC) + Linux (GCC/Clang) + Android (NDK)
- [ ] **2.8** SPIR-V 路径检查：确认 `cmake/FetchThirdPartyDeps.cmake` 中无硬编码 `spirv-headers`

### Phase 3 — 渐进式特性采用（按优先级）

- [ ] **3.1** 引入 `std::expected` 替换错误处理（OBJParser、Vulkan 资源创建）
- [ ] **3.2** 替换 5 个最热 `std::unordered_map` 为 `std::flat_map`（Shader 缓存、Material 注册、RenderGraph）
- [ ] **3.3** 日志系统切换到 `std::print`（替换 `printf`/`cout`）
- [ ] **3.4** 使用 `deducing this` 简化 `Singleton<T>` CRTP 模式
- [ ] **3.5** 在 `Prisma.Core.csproj` 启用 `runtime-async=on`（如果 C# 脚本使用 async）

### Phase 4 — 回归测试与文档

- [ ] **4.1** 运行全部示例项目：Template2D / Template3D / PacManGame / PrismaCraft
- [ ] **4.2** Asset 加载测试（确保 NBT 序列化 / 资源反序列化不受 C++23 影响）
- [ ] **4.3** 运行 Android APK 构建并验证 Vulkan 渲染
- [ ] **4.4** 更新 8 个文档文件 + CLAUDE.md / GEMINI.md / README.md
- [ ] **4.5** 更新 `cmake/DEPENDENCY_VERSIONING.md` 依赖兼容性矩阵

---

## 5. 风险与缓解

| 风险 | 概率 | 影响 | 缓解措施 |
|------|------|------|----------|
| Glaze/vk-bootstrap 在 C++23 下编译失败 | 低 | 高 | 先行验证：在 GCC 15 上编译 Glaze v7.5.0 + vk-bootstrap v1.4.351 |
| .NET 11 HostFxr API 二进制不兼容 | 极低 | 高 | 官方兼容性列表确认无变更；先行在 Termux 验证 |
| Android NDK Clang 不支持 C++23 核心特性 | 低 | 中 | NDK r27+ 自带 Clang 18+ → 覆盖 `expected`/`mdspan`/`flat_map` |
| Vulkan 1.4.351 在 Android 驱动上不可用 | 极低 | 低 | SDK 1.4.351 向下兼容 1.3 API；引擎仍使用 `VK_API_VERSION_1_3` |
| `.github/workflows/ci-linux.yml` Ubuntu 镜像不兼容 | 中 | 中 | SDK 1.4.351 要求 Ubuntu 24.04；CI runner 需从 `ubuntu-22.04` 升级 |
| WiX 6.0 不支持 .NET 11 SDK | 低 | 低 | 安装程序构建可临时使用独立 .NET 6 SDK，或升级 WiX |
| `std::generator` Clang 不支持 | 高 | 低 | 暂缓采用，使用传统回调模式替代 |

---

## 6. 文件影响总览

| 类别 | 文件数 | 风险 |
|------|--------|------|
| P0 - CMake 构建配置 | 10 | 中 |
| P0 - .csproj + 构建路径 | 8 | 低 |
| P0 - Vulkan 依赖 | 3 | 极低 |
| P0 - C++ Vulkan API 版本 | 3 | 无操作 |
| P1 - CI/CD 工作流 | 5 | 中（需 Ubuntu 升级） |
| P2 - 文档 + README | 9 | 无 |
| **合计** | **~38** | — |

---

## 7. 验证标准

全部通过方可视为迁移完成：

- [x] Phase 1 完成 — 所有平台工具链可用
- [ ] Phase 2 完成 — 零修改全平台编译通过
- [ ] 全部示例项目运行无异常
- [ ] Android APK 构建成功 + Vulkan 验证层零错误
- [ ] 引擎 SDK ARM64 发布构建成功
- [ ] 文档全部更新
- [ ] 勿删除 `DOTNET_GCRegionRange=0x10000000` Termux 构建说明
