# GitHub Actions Workflows

这个目录包含项目的CI/CD工作流配置。

## 工作流说明

### 工作流选择策略

为了优化CI/CD效率，我们采用了互斥的workflow设计：

1. **Quick Workflow** - 日常开发验证
   - 当修改了 `src/engine/`、`android/` 或 `vcpkg.json` 时触发
   - 快速验证代码编译是否正常

2. **Complete Workflow** - 发布和完整测试
   - 当修改了非上述路径时触发（如文档、配置等）
   - 发布标签时自动触发
   - 手动触发时使用

### 1. build-android.yml - 完整Android构建

触发条件：
- 推送到 main、develop 分支（当不触发quick workflow时）
- 创建标签（v开头的标签）
- 对 main 分支的 Pull Request（当不触发quick workflow时）
- 手动触发（workflow_dispatch）

特性：
- **多架构支持**：并行构建 arm64-v8a、armeabi-v7a
- **多构建类型**：同时构建 Release 和 Debug 版本
- **智能缓存**：
  - Android SDK 缓存（约 1GB）
  - Android NDK 缓存（约 800MB）
  - vcpkg 依赖缓存（按架构分别缓存）
- **自动发布**：创建标签时自动生成 GitHub Release
- **构建产物**：自动上传构建结果作为 GitHub Artifacts

### 2. build-android-quick.yml - 快速Android构建

触发条件：
- 推送到 main、develop 分支（仅当相关文件变化时）
- 对 main 分支的 Pull Request（仅当相关文件变化时）

监控的路径：
- `src/engine/**` - 引擎源代码
- `android/**` - Android特定配置
- `vcpkg.json` - 依赖配置

特性：
- **快速验证**：只构建 arm64-v8a Release 版本
- **轻量级**：使用最小化的依赖
- **路径过滤**：避免不必要的构建
- **并发控制**：新推送会取消正在进行的构建

## 缓存策略

### Android SDK/NDK 缓存
- SDK 缓存键：`android-sdk-${version}-v2`
- NDK 缓存键：`android-ndk-${version}-v1`
- 缓存有效期：30天（GitHub Actions默认）

### vcpkg 缓存
- 缓存键：`vcpkg-android-${abi}-${hash}`
- 按架构分别缓存，避免交叉污染
- 包含所有预编译的依赖库

## 性能优化

1. **并行构建**：使用 matrix strategy 并行构建多个架构
2. **增量构建**：利用 CMake 和 ninja 的增量构建能力
3. **依赖缓存**：避免重复下载和编译大型依赖
4. **条件触发**：quick workflow 使用路径过滤减少不必要的构建

## 使用方法

### 本地运行与CI一致的构建

```bash
# 设置环境变量（根据你的实际路径）
export ANDROID_NDK_HOME=/path/to/android-ndk-r27
export ANDROID_HOME=/path/to/android-sdk

# 构建arm64-v8a
./scripts/build-android.sh --abi arm64-v8a

# 构建所有架构
./scripts/build-android.sh --all
```

### 手动触发完整构建

在 GitHub Actions 页面：
1. 进入 "Actions" 标签页
2. 选择 "Build Android" workflow
3. 点击 "Run workflow"
4. 选择构建参数（可选）

## 构建产物

### GitHub Artifacts
- 文件命名：`libEngine-{abi}-{build_type}.tar.gz`
- 包含内容：编译好的 `.so` 文件和头文件
- 保留时间：30天

### GitHub Release（仅标签触发）
- 文件命名：`PrismaEngine-Android-{version}.tar.gz`
- 包含内容：
  - 所有架构的 `.so` 文件（按Android标准目录结构）
  - 公共头文件
- 与版本标签关联

## 故障排除

### 常见问题

1. **缓存失效**
   - 如果构建失败，可以手动清除缓存
   - 在 Actions 页面 -> Settings -> Caches 中管理

2. **NDK下载失败**
   - 检查 NDK 版本是否正确
   - 确认网络连接正常

3. **vcpkg编译错误**
   - 可能是依赖版本冲突
   - 检查 `vcpkg.json` 配置

4. **构建超时**
   - GitHub Actions 有时间限制（默认6小时）
   - 大型项目可能需要优化构建步骤

### 调试技巧

1. 使用 `workflow_dispatch` 手动触发，可以：
   - 选择特定的构建类型
   - 控制是否上传构建产物

2. 查看详细的构建日志：
   - 注意 "Cache hit/miss" 信息
   - 检查环境配置是否正确

3. 本地复现问题：
   - 使用与CI相同的脚本
   - 确保环境变量一致