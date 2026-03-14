# Prisma Engine Code Review - The Cherno's Perspective

> "Make it work, make it right, make it fast, and make it SEXY." - The Cherno (probably)

这份文档汇总了 Prisma Engine 当前架构的深度评审结果。我们将按照模块化的方式，指出当前代码中的设计缺陷、命名晦涩、职责不清以及性能隐患，并提供具体的架构重构建议。

---

## 1. 核心架构与生命周期 (Core Architecture & Lifecycle)

### [现状评审]
经过最近的重构，`Prisma::Engine` 和 `Prisma::Application` 已经建立。引入了 `LayerStack`、强类型 `Event` 系统和 `Timestep` 类。这标志着引擎已经从“过程式代码”向“架构化框架”转变。

### [发现的问题]
1.  **控制流的语义模糊**：虽然 `Engine::Run(app)` 已经建立，但 `Launcher` 依然存在过多的“手动挡”操作（例如手动销毁 app）。
2.  **事件系统的 NativeEvent 暴露**：在 `Event` 类中暴露 `void* NativeEvent` 是一种危险的妥协。它让 Layer 层能够越过抽象直接操作 SDL 事件，这违反了我们建立“无平台”引擎的初衷。
3.  **单例模式的滥用**：`Engine` 和 `Application` 都使用了单例。虽然这在引擎中常见，但要注意所有权。目前 `Launcher` 拥有 `app` 的 `delete` 权，而 `Engine` 引用了 `app`。这种交错的所有权容易导致悬空指针。

### [优化建议]
- **智能指针所有权**：`Engine` 应该持有一个 `std::unique_ptr<Application>`。`Launcher` 只负责 `CreateApplication()`，然后将其 `std::move` 给引擎。引擎死，应用死。
- **事件完全脱敏**：彻底移除 `NativeEvent`。Platform 层应该在转换事件时提取所有必要数据存入强类型字段（例如 `Key`, `Mod`, `Button`, `Position`），而不是传递原始指针。
- **命名一致性**：`IApplication.h` 文件名应改为 `Application.h`。

---

## 2. 资源管理系统 (Resource Management) - **重灾区**

### [现状评审]
代码中充斥着：
- `ResourceManager.cpp/h`
- `core/AssetManager.cpp/h`
- `core/AsyncLoader.cpp/h`
- `core/ResourceManagerV2.h`
- `core/AsyncResourceLoaderV3.cpp`
- `core/AsyncResourceLoaderV2.cpp`

### [发现的问题]
1.  **冗余与混乱**：V2, V3 并存，且文件分布极其零散。这说明开发者在开发新功能时没有进行存量重构，而是不断开新坑。
2.  **职责重叠**：`AssetManager` 和 `ResourceManager` 的界限是什么？
3.  **缺乏统一入口**：没有一个全局唯一的资源访问点，导致渲染层、脚本层都在用不同的方式加载纹理。

### [优化建议]
- **大一统重构**：**废弃并删除** 所有 V2, V3 以及分散的 Loader。
- **定义 Asset 抽象**：建立 `Prisma::Asset` 基类（UUID, Type, State）。
- **建立单一入口**：创建一个 `Prisma::AssetManager`。它应该是引擎的一个 `SubSystem`。
- **异步机制合并**：将所有的 AsyncLoader 逻辑整合进 `AssetManager`，使用 `JobSystem` 来驱动加载任务。

---

## 3. 渲染系统 (Rendering System)

### [现状评审]
渲染系统采用了单线程推送模型，基于 Vulkan。支持 RenderGraph (初步) 和 多 Pass 架构。

### [发现的问题]
1.  **RenderAPIVulkan 的膨胀**：该文件目前已经超过 800 行。它既处理实例创建，又处理内存管理，还处理具体的渲染通路逻辑。
2.  **缺乏命令记录抽象**：`OnRender` 逻辑目前太底层。
3.  **Vulkan 资源手动释放**：很多地方还在手动 `vkDestroy...`。在 Vulkan 引擎中，我们需要一个 `DeletionQueue`。

### [优化建议]
- **引入 Renderer API 层**：借鉴 Hazel，不要在 `RenderSystem` 里直接写 `Vk...`。建立 `RendererAPI` -> `VulkanRendererAPI`。
- **渲染命令抽象**：引入 `RenderCommand` 静态类。
- **引入 DeletionQueue**：创建一个延迟释放队列。Vulkan 资源必须在 GPU 确认不再使用（通过 Fence）后，由 DeletionQueue 统一销毁。
- **Shader 编译自动化**：目前代码中看到手动加载编译后的二进制。应该集成一个简单的运行时 Shader 编译（利用 glslang）。

---

## 4. 平台与输入 (Platform & Input)

### [现状评审]
建立了 `Platform` 抽象，移除了宏。

### [发现的问题]
1.  **输入系统未对齐**：`InputManager` 目前还是旧的实现，它并没有完全基于我们的新 `Event` 系统。
2.  **窗口状态同步**：`Platform::PumpEvents` 只是翻译事件，但没有一个集中的 `WindowState` 结构体来缓存当前的宽高、焦点状态。

### [优化建议]
- **重写 Input 类**：建立一个静态的 `Prisma::Input` 类，提供 `IsKeyPressed(KeyCode)`。它应该维护一个 `std::bitset` 来缓存按键状态，这个状态位由 `OnEvent` 自动更新。
- **Window 封装**：将 `Platform::CreateWindow` 返回的 `void*` 封装成一个 `Prisma::Window` 对象。

---

## 5. 项目结构与构建系统 (Project Structure & Build)

### [现状评审]
`src/game` 正在被清理。`CMakeLists.txt` 已更新。

### [发现的问题]
1.  **代码污染**：`PrismaCraft` (Sample) 不应该在 `src/` 目录下。这会导致引擎源码和应用源码混在一起。
2.  **CMake 复杂度**：目前的 CMake 逻辑过于分散。

### [优化建议]
- **隔离 Samples**：将 `PrismaCraft` 移至根目录下的 `samples/PrismaCraft`。它应该作为一个完全独立的工程，通过 `find_package(Prisma)` 或者 `add_subdirectory(engine)` 的方式引用引擎。
- **建立 Sandbox 模式**：创建一个 `Sandbox` 目录，用于存放日常测试代码。

---

## 6. 开发者体验 (DevEx)

### [现状评审]
缺乏调试工具。

### [优化建议]
- **加强 DebugOverlay**：利用 ImGui 建立一个实时的 Subsystem 状态监控器。
- **自动化资源统计**：在 `AssetManager` 中建立内存占用监控。

---

**Cherno 的最终结语：**
"Prisma Engine 的大梁已经架好了，现在的首要任务是**清理冗余逻辑（尤其是资源管理器）**并**深化 Vulkan 层的抽象**。不要害怕删代码，现在的冗余是以后性能和维护的杀手。"
