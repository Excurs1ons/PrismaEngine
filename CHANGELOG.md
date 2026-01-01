# Changelog / 变更日志

All notable changes to Prisma Engine will be documented in this file.

Prisma Engine 的所有重要变更都将记录在此文件中。

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

格式基于 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.0.0/)，
本项目遵循 [语义化版本](https://semver.org/lang/zh-CN/spec/v2.0.0.html)。

## [Unreleased]

### Added / 新增

- Android runtime support with Vulkan rendering backend / Android 运行时支持，使用 Vulkan 渲染后端
- Cross-platform resource management system / 跨平台资源管理系统
- Unified shader resource directory structure (`resources/common/shaders/`) / 统一的着色器资源目录结构
- Automatic GLSL to SPIR-V compilation for Android / Android 自动 GLSL 到 SPIR-V 编译
- Gradle asset copying task for Android builds / Android 构建的 Gradle 资产复制任务

### Changed / 变更

- **BREAKING**: Renamed `Engine` namespace to `PrismaEngine` / **破坏性变更**：将 `Engine` 命名空间重命名为 `PrismaEngine`
- **BREAKING**: Unified math types in `MathTypes.h` / **破坏性变更**：统一 `MathTypes.h` 中的数学类型
- **BREAKING**: Moved `ResourceManager` to `AssetManager` / **破坏性变更**：将 `ResourceManager` 移至 `AssetManager`
- **BREAKING**: Reorganized resource directory structure / **破坏性变更**：重组资源目录结构
- Migrated `Camera3D` to `Camera` (removed 2D camera) / 将 `Camera3D` 迁移到 `Camera`（移除 2D 相机）
- Refactored component system with unified `Components.h` / 使用统一的 `Components.h` 重构组件系统
- Updated InputManager to use ManagerBase pattern / 更新 InputManager 使用 ManagerBase 模式
- Improved rendering descriptor structures (`RenderDesc.h`) / 改进渲染描述符结构
- Enhanced Android shader loading with automatic compilation / 增强 Android 着色器加载，支持自动编译

### Removed / 移除

- `Camera2D` class and related files / `Camera2D` 类及相关文件
- `Camera3DController` (merged into `CameraController`) / `Camera3DController`（合并到 `CameraController`）
- `Transform2D` class and related files / `Transform2D` 类及相关文件
- `Quaternion` and `Vector3` classes (use GLM types) / `Quaternion` 和 `Vector3` 类（使用 GLM 类型）
- `Color` and `Math` classes (moved to `MathTypes.h`) / `Color` 和 `Math` 类（移至 `MathTypes.h`）
- `ResourceManager` old implementation / `ResourceManager` 旧实现
- Old `android/` directory configuration / 旧的 `android/` 目录配置

### Fixed / 修复

- Fixed CMake `CMAKE_ROOT` reference in FetchContent.cmake / 修复 FetchContent.cmake 中的 CMAKE_ROOT 引用
- Fixed platform-specific runtime build configurations / 修复平台特定的运行时构建配置

### Documentation / 文档

- Added `DirectoryStructure.md` (bilingual) / 添加 `DirectoryStructure.md`（双语）
- Added `ResourceManager.md` (bilingual) / 添加 `ResourceManager.md`（双语）
- Updated `CLAUDE.md` with cross-platform information / 更新 `CLAUDE.md` 包含跨平台信息
- Updated `RenderingSystem.md` (bilingual) / 更新 `RenderingSystem.md`（双语）
- Updated `VulkanIntegration.md` (bilingual) / 更新 `VulkanIntegration.md`（双语）

## [0.1.0] - 2024-12-XX

### Added / 新增

- Initial Windows platform support / 初始 Windows 平台支持
- DirectX 12 rendering backend / DirectX 12 渲染后端
- Basic asset management system / 基本资产管理系统
- ECS (Entity Component System) / ECS（实体组件系统）
- Audio system with XAudio2 / 使用 XAudio2 的音频系统
- SDL3 integration for windowing and input / SDL3 集成用于窗口和输入
- ImGui-based editor / 基于 ImGui 的编辑器

## Version Format / 版本格式

- **Major.Minor.Patch** (主版本.次版本.补丁)
- Major: Breaking changes / 主版本：破坏性变更
- Minor: New features (backwards compatible) / 次版本：新功能（向后兼容）
- Patch: Bug fixes / 补丁：错误修复

## Related Documentation / 相关文档

- [Directory Structure](docs/DirectoryStructure.md) - Project organization / 项目组织
- [Rendering System](docs/RenderingSystem.md) - Rendering architecture / 渲染架构
- [Resource Management](docs/ResourceManager.md) - Asset loading / 资产加载
- [Android Integration](docs/VulkanIntegration.md) - Android setup / Android 设置
