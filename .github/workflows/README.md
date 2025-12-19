# GitHub Actions Workflows

这个目录包含Prisma Engine的CI/CD工作流配置。我们已经简化为单一的、智能化的构建系统。

## 工作流架构

### 核心原则

1. **智能构建**：基于文件路径自动检测需要构建的组件
2. **统一管理**：所有构建逻辑集成在一个workflow中
3. **按需触发**：只在相关代码变更时触发相应构建
4. **自动化发布**：创建标签时自动生成发布包

## 工作流说明

### Build All Components (`build-all.yml`)

这是唯一的workflow文件，负责所有构建任务。

**触发条件**：
- 推送到任何分支
- 创建版本标签（`v*`）
- Pull Request
- 手动触发（可选择组件）

**智能检测机制**：
- `src/editor/` 变更 → 构建Editor
- `src/engine/` 或 `android/` 变更 → 构建Engine
- `src/game/` 或 `src/runtime/` 变更 → 构建Game
- 标签创建 → 构建所有组件

**手动触发选项**：
- 选择构建的组件（Editor/Engine/Game）
- 选择平台（Windows/Android/全部）
- 选择构建类型（Release/Debug）
- 是否上传构建产物

## 构建组件详情

### 1. Editor 构建

**平台**：Windows (x64, x86)

**输出**：
- `PrismaEditor-win64-Release.zip` - Windows 64位版本
- `PrismaEditor-win32-Release.zip` - Windows 32位版本
- Debug版本也相应生成

**内容**：
- Editor可执行文件
- 所需DLL文件
- 资源文件

### 2. Engine 构建

**Windows平台**：
- 静态库（.lib）
- 头文件
- CMake配置文件
- 输出：`PrismaEngine-win64-Release.zip`等

**Android平台**：
- 动态库（.so）
- 支持ARM64和ARMv7
- 输出：`PrismaEngine-android-arm64-Release.tar.gz`等

### 3. Game 构建

**平台**：Windows (x64)

**输出**：
- `PrismaGame-x64-Release.zip` - 游戏运行时
- 包含Runtime.exe和相关资源

## 使用方法

### 日常开发

**编辑器开发**：
```bash
# 修改src/editor/中的文件
# 自动触发Editor构建
git add .
git commit -m "Update editor"
git push
```

**引擎开发**：
```bash
# 修改src/engine/中的文件
# 自动触发Engine构建
git add .
git commit -m "Update engine core"
git push
```

**游戏开发**：
```bash
# 修改src/game/或src/runtime/中的文件
# 自动触发Game构建
git add .
git commit -m "Update game logic"
git push
```

### 手动触发构建

1. 访问GitHub仓库的Actions页面
2. 选择"Build All Components"工作流
3. 点击"Run workflow"
4. 选择需要的参数：
   - 勾选要构建的组件
   - 选择目标平台
   - 选择构建类型
   - 设置是否上传产物

### 发布版本

创建标签会自动触发完整构建和发布：

```bash
# 创建并推送标签
git tag v1.0.0
git push origin v1.0.0
```

自动执行：
- 构建所有组件
- 创建GitHub Release
- 上传所有构建产物
- 生成发布说明

## 构建产物

### GitHub Artifacts

- **保留期限**：30天
- **位置**：Actions页面的Artifacts部分
- **内容**：各组件的zip/tar.gz文件

### GitHub Release

创建标签时自动生成：
- **完整包**：`PrismaEngine-Complete-{version}.tar.gz`
- **包含**：所有组件和详细说明文档

## 性能特性

### 智能缓存
- **vcpkg缓存**：按平台和构建类型分别缓存依赖
- **增量构建**：只编译变更的部分
- **避免重复下载**：NDK、SDK等大文件缓存

### 并行优化
- 多平台同时构建
- 多架构并行（Android）
- 矩阵策略优化资源使用

## 故障排除

### 常见问题

1. **构建失败**
   - 检查具体组件的构建日志
   - 查看是否有编译错误
   - 确认依赖项配置

2. **缓存问题**
   - Actions设置中清除缓存
   - 重新触发构建
   - 检查vcpkg.json

3. **发布失败**
   - 确认标签格式（v开头）
   - 检查GITHUB_TOKEN权限
   - 查看Release创建日志

### 调试步骤

1. **查看详细日志**
   - Actions页面 → 具体运行 → Jobs
   - 点击失败的job查看详细信息

2. **本地复现**
   ```bash
   # Windows
   cmake -B build -G "Visual Studio 17 2022" -A x64
   cmake --build build --config Release

   # Android
   ./scripts/build-android.sh --abi arm64-v8a
   ```

3. **联系支持**
   - 在仓库中创建Issue
   - 提供完整的错误信息
   - 附加构建日志片段

## 维护建议

### 定期任务
- 清理过期的Artifacts（GitHub会自动删除30天后的）
- 更新依赖版本（vcpkg, NDK等）
- 优化构建性能

### 添加新功能
1. 如需支持新平台
   - 在build-all.yml中添加matrix条目
   - 添加平台特定的构建步骤
   - 更新文档

2. 如需新组件
   - 添加变更检测逻辑
   - 添加构建job
   - 配置产物上传

---

这个简化的设计提供了更好的可维护性，同时保持了所有必要的功能。