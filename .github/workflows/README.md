# GitHub Actions Workflows

这个目录包含Prisma Engine的CI/CD工作流配置。我们已经重构为模块化的构建系统，分为三个主要组件。

## 工作流架构

### 核心原则

1. **模块化构建**：每个组件独立构建，只在相关代码变更时触发
2. **智能触发**：基于文件路径自动检测需要构建的组件
3. **平台支持**：Windows（主要平台）和Android（移动平台）
4. **自动化发布**：创建标签时自动生成发布包

## 工作流说明

### 1. Build Editor (`build-editor.yml`)

**触发条件**：
- 推送到 `src/editor/` 目录
- 对 `src/editor/` 的Pull Request
- 手动触发

**构建内容**：
- Windows Editor应用程序（x64, x86）
- Visual Studio 2022项目
- 所有依赖项（通过vcpkg）

**输出**：
- `PrismaEditor-win64-Release.zip` - Windows 64位版本
- `PrismaEditor-win32-Release.zip` - Windows 32位版本

**特性**：
- 完整的编辑器功能
- 集成开发工具
- 资源管理器
- 场景编辑器

### 2. Build Engine (`build-engine.yml`)

**触发条件**：
- 推送到 `src/engine/` 或 `android/` 目录
- 对这些目录的Pull Request
- 手动触发

**构建内容**：
- Windows静态库（SDK）
- Android动态库
- 头文件和CMake配置

**输出**：
- **Windows**: `PrismaEngine-win64-Release.zip`, `PrismaEngine-win32-Release.zip`
- **Android**: `PrismaEngine-android-arm64-v8a-Release.tar.gz`, `PrismaEngine-android-armeabi-v7a-Release.tar.gz`

**特性**：
- 跨平台引擎核心
- 渲染系统
- 音频系统
- 资源管理

### 3. Build Game (`build-game.yml`)

**触发条件**：
- 推送到 `src/game/` 或 `src/runtime/` 目录
- 对这些目录的Pull Request
- 手动触发

**构建内容**：
- Windows游戏运行时
- Android游戏项目模板
- 示例项目

**输出**：
- **Windows**: `PrismaGame-x64-Release.zip`
- **Android**: `PrismaGame-android-*-Release.tar.gz`

**特性**：
- 独立游戏运行时
- 项目模板
- 示例内容

### 4. Build All (`build-all.yml`)

**触发条件**：
- 推送到任何分支
- 创建版本标签
- Pull Request
- 手动触发（可选择组件）

**功能**：
- 智能检测变更的组件
- 并行触发需要的构建
- 创建统一的发布包
- 生成完整的GitHub Release

## 使用方法

### 日常开发

1. **编辑器开发**：
   - 修改 `src/editor/` 中的文件
   - 自动触发Editor构建
   - 快速获取Editor可执行文件

2. **引擎开发**：
   - 修改 `src/engine/` 中的文件
   - 自动触发Engine构建
   - 获取Windows SDK和Android库

3. **游戏开发**：
   - 修改 `src/game/` 或 `src/runtime/`
   - 自动触发Game构建
   - 获取游戏运行时

### 手动触发

在GitHub Actions页面可以手动触发构建：

1. 进入Actions页面
2. 选择需要的工作流
3. 点击"Run workflow"
4. 选择参数：
   - 平台（Windows/Android/全部）
   - 构建类型（Release/Debug）
   - 是否上传构建产物

### 发布版本

创建标签时自动触发完整构建和发布：

```bash
# 创建发布标签
git tag v1.0.0
git push origin v1.0.0
```

自动执行：
- 构建所有组件
- 创建GitHub Release
- 上传所有构建产物
- 生成发布说明

## 构建产物

### Artifacts

所有构建产物都会上传为GitHub Artifacts：
- 保留期限：30天
- 可从Actions页面下载
- 包含完整的使用说明

### Release包

创建标签时生成：
- `PrismaEngine-Complete-{version}.tar.gz` - 包含所有组件
- 自动生成Release页面
- 包含详细的安装说明

## 性能优化

### 智能缓存
- **vcpkg缓存**：避免重复编译依赖
- **NDK缓存**：Android NDK下载缓存
- **增量构建**：只编译变更的部分

### 并行构建
- 多平台并行构建
- 多架构并行构建（Android）
- 矩阵策略优化资源使用

### 条件触发
- 基于文件路径的智能触发
- 避免不必要的构建
- 节省CI/CD资源

## 故障排除

### 常见问题

1. **构建失败**
   - 检查代码编译错误
   - 查看构建日志
   - 确认依赖项版本

2. **缓存问题**
   - 清除Actions缓存
   - 重新触发构建
   - 检查vcpkg.json配置

3. **发布问题**
   - 确认标签格式（v开头）
   - 检查GITHUB_TOKEN权限
   - 查看Release创建日志

### 调试技巧

1. **本地复现**
   ```bash
   # 设置环境
   export ANDROID_NDK_HOME=/path/to/ndk
   export ANDROID_HOME=/path/to/sdk

   # 构建命令
   ./scripts/build-android.sh
   ```

2. **查看详细信息**
   - Actions页面查看完整日志
   - 注意缓存命中/miss信息
   - 检查环境配置

3. **联系支持**
   - 在Issues中报告问题
   - 提供详细的错误信息
   - 附上构建日志片段

## 贡献指南

当添加新功能或修复问题时：

1. 确定修改的组件（Editor/Engine/Game）
2. 在相应的目录中进行修改
3. 提交PR会自动触发相应的构建
4. 确保所有构建通过

### 添加新平台支持

1. 更新相应的workflow文件
2. 添加平台特定的构建步骤
3. 更新文档和测试
4. 提交PR进行审核