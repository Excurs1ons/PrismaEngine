# 依赖版本锁定说明 / Dependency Version Locking

## 概述 / Overview

Prisma Engine 使用 CMake FetchContent 管理第三方依赖。为了确保构建的可重复性和稳定性，我们引入了版本锁定机制。

## 核心文件 / Core Files

### 1. `cmake/DependencyVersions.cmake`
定义所有依赖的版本号。这是**唯一**需要修改来更新依赖版本的文件。

### 2. `cmake/FetchThirdPartyDeps.cmake`
使用 `DependencyVersions.cmake` 中定义的版本号来下载依赖。

### 3. `scripts/update-deps.sh`
安全更新依赖版本的工具脚本。

### 4. `scripts/check-deps.sh`
检查依赖缓存状态的工具脚本。

## 版本锁定策略 / Version Locking Strategy

### 更新策略控制

通过 `FETCHCONTENT_UPDATES_DISCONNECTED` 变量控制：

```cmake
# 默认行为：锁定版本，不自动更新
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

# 如果需要更新，设置为 OFF
set(FETCHCONTENT_UPDATES_DISCONNECTED OFF)
```

### 版本类型优先级

1. **Release Tag** (优先) - 如 `v1.0.0`, `release-3.2.28`
2. **Commit SHA** - 如 `af1a5bc352164740c1cc1354942b1c6b72eacb8a`
3. **Branch Name** (避免) - 如 `master`, `main`, `docking`

### 当前锁定的依赖

| 依赖 | 版本 | 类型 |
|------|------|------|
| GLM | 1.0.2 | Tag |
| nlohmann_json | v3.12.0 | Tag |
| stb | af1a5bc... | Commit |
| tinyxml2 | 10.0.0 | Tag |
| zstd | v1.5.6 | Tag |
| Tweeny | 3.2.0 | Tag |
| SDL3 | release-3.2.28 | Tag |
| Vulkan-Headers | v1.4.328 | Tag |
| VMA | v3.1.0 | Tag |
| vk-bootstrap | v0.9 | Tag |
| DirectX-Headers | v1.614.1 | Tag |
| ImGui | docking | Branch |
| OpenFBX | c2ac836... | Commit |
| FidelityFX-SDK | v2.1.0 | Tag |
| Streamline | v2.9.0 | Tag |

## 使用方法 / Usage

### 查看当前版本

```bash
./scripts/update-deps.sh --list
```

### 检查依赖状态

```bash
./scripts/check-deps.sh
```

### 更新依赖版本

```bash
# 更新 GLM 到新版本
./scripts/update-deps.sh --update GLM 1.0.2

# 更新 SDL3 到新版本
./scripts/update-deps.sh --update SDL3 release-3.2.28

# 更新 STB 到指定 commit
./scripts/update-deps.sh --update STB af1a5bc352164740c1cc1354942b1c6b72eacb8a
```

### 测试依赖更新

```bash
./scripts/update-deps.sh --test
```

### 清理依赖缓存

```bash
# 删除所有依赖缓存
rm -rf build/_deps

# 删除特定依赖缓存
rm -rf build/_deps/SDL3-src
```

### 强制重新下载依赖

```bash
cmake --preset windows-x64-debug -DFETCHCONTENT_UPDATES_DISCONNECTED=OFF
```

## 更新流程 / Update Workflow

1. **检查更新**
   ```bash
   ./scripts/update-deps.sh --check
   ```

2. **更新版本**
   ```bash
   ./scripts/update-deps.sh --update <DEP> <VERSION>
   ```

3. **测试编译**
   ```bash
   ./scripts/update-deps.sh --test
   ```

4. **检查警告和错误**
   - 查看编译输出中的 deprecation warnings
   - 检查 API 变更

5. **更新验证日期**
   - 脚本会自动更新 `PRISMA_DEP_LAST_VERIFIED`

6. **提交更改**
   ```bash
   git add cmake/DependencyVersions.cmake
   git commit -m "chore(deps): update SDL3 to release-3.2.28"
   ```

## 依赖兼容性矩阵

| 引擎版本 | 依赖矩阵版本 | 验证日期 |
|----------|-------------|----------|
| 1.0.0 | 1.0.0 | 2025-01-24 |

## 已知问题 / Known Issues

- SDL3 3.2.28+: Android 构建需要 NDK r25+
- VMA 3.1.0: 需要 Vulkan-Headers 1.3.231+
- ImGui docking: 频繁更新，建议定期检查兼容性

## 故障排除 / Troubleshooting

### 依赖版本不匹配

如果遇到版本不匹配错误：

```bash
# 清理所有缓存
rm -rf build/

# 重新配置
cmake --preset windows-x64-debug
```

### 强制使用特定版本

如果需要临时使用不同版本：

```bash
cmake --preset windows-x64-debug \
    -DFETCHCONTENT_SOURCE_DIR_SDL3=/path/to/SDL3
```

### 检查实际下载的版本

```bash
./scripts/check-deps.sh
```

## 最佳实践 / Best Practices

1. **定期检查更新** - 每月检查一次依赖是否有安全更新
2. **锁定版本** - 生产环境始终使用版本锁定
3. **测试更新** - 在测试环境验证依赖更新
4. **文档记录** - 在 `PRISMA_DEP_KNOWN_ISSUES` 中记录问题
5. **备份配置** - 更新前备份 `DependencyVersions.cmake`

## CI/CD 集成

```yaml
# GitHub Actions 示例
- name: Check Dependencies
  run: ./scripts/check-deps.sh

- name: Build with Locked Versions
  run: cmake --preset windows-x64-release
```

## 相关链接 / Related Links

- [CMake FetchContent 文档](https://cmake.org/cmake/help/latest/module/FetchContent.html)
- [依赖管理策略](../docs/DependencyManagement.md)
