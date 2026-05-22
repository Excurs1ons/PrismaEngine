# Visual Studio 2026 迁移与依赖适配性设计文档

## 1. 目标
将 Prisma Engine 的 Windows 构建环境从 Visual Studio 2022 迁移到最新的 Visual Studio 2026，同时验证所有第三方依赖库在最新编译器（MSVC v144 或其对应版本）下的兼容性。

## 2. 依赖适配性检查分析
目前项目在 `cmake/DependencyVersions.cmake` 中通过 FetchContent 锁定了多个第三方依赖：
- **C++20 核心与图形依赖**：GLM (1.0.2), nlohmann_json (v3.12.0), Glaze (v7.5.0), stb, tinyxml2 (10.0.0), zstd (v1.5.6) 等，皆为主流的 C++ 库，且在 C++20 模式下能够原生良好支持最新版本的 MSVC 编译器，没有已知的阻断性不兼容。
- **SDL3 & Vulkan**：`SDL3 (release-3.2.28)`、`Vulkan-Headers (v1.4.328)` 以及对应的 `VMA` 和 `vk-bootstrap`，属于活跃维护状态的跨平台代码，全面兼容较新的编译前端。
- **结论**：当前使用的锁定版本本身已经过良好的测试，可以平滑过渡至 Visual Studio 2026 进行编译。如有警告，由 `CompilerOptions.cmake` 统一管理和静默。

## 3. 实施计划

### 3.1. CMake 配置更新
- **文件**：`CMakePresets.json`
- **变更**：将 Windows 基础配置中的 `"generator": "Visual Studio 17 2022"` 更新为 `"generator": "Visual Studio 18 2026"`（或对应 VS2026 的正确生成器名）。

### 3.2. 构建系统检查
- **文件**：`cmake/CompilerOptions.cmake`
- **说明**：当前脚本通过匹配 `CMAKE_GENERATOR MATCHES "Visual Studio"` 进行通用判定，`CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION` 亦已配置为 `"latest"`，该逻辑直接兼容新版生成器，无需调整。

### 3.3. 脚本与文档全局更新
项目中包含大量的 VS2022 环境搭建指引与硬编码参考，需要全部升级为 Visual Studio 2026：
- **文档类**：`CLAUDE.md`, `docs/MEMO.md`, `docs/ScriptingSystem.md`, `docs/RenderingRefactoringPlan.md`, `docs/DeviceConfiguration.md`, `docs/MCPTestReport.md`, `sdk/README.md`, `.github/workflows/README.md`
- **脚本类**：`scripts/README.md`, `scripts/package-sdk.sh`, `scripts/setup-env.ps1`
- **忽略项**：第三方或原作者的版权声明（如 `src/editor/core/webview.h` 中的 `Copyright (c) 2022`）保持不变。

## 4. 验证测试
- 在替换完成后，引导开发者在本地通过 `./scripts/build-windows.bat` 测试新生成器的工程生成与编译。
