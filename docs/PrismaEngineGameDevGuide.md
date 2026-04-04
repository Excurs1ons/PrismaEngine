# PrismaEngine 游戏开发指南（Prune 分支）

本指南基于 `prune` 分支的实际开发与排障过程整理，目标是让项目可在 SDL3 + Vulkan 适配平台上稳定开发、构建与交付。

## 1. 开发基线

- 渲染后端：Vulkan
- 平台层：SDL3
- 分支目标：尽可能减少平台特定 API，统一通过 SDL3 + Vulkan 适配

建议：
- 游戏逻辑只依赖引擎抽象层，不直接触碰平台 API。
- 平台能力判断优先走 `Platform` 封装（例如显示设备检测）。

## 2. 构建策略（推荐）

统一使用脚本自动识别平台与架构：

```bash
./scripts/build.sh
./scripts/build.sh --target pacman --config debug
```

PacMan 独立构建（已拆分输出）：
- `build/pacman-linux-x64-debug`
- `build/pacman-linux-x64-release`
- `build/pacman-linux-arm64-debug`
- `build/pacman-linux-arm64-release`

这能避免游戏产物混入 editor 构建目录。

## 3. 运行与交付

开发运行：

```bash
./scripts/run-pacman.sh --config debug
```

发布打包：

```bash
./scripts/package-pacman.sh --config release
```

无图形设备时：
- 游戏会自动进入 console view（文本输入输出）。
- 用于服务器/容器/无桌面设备下的逻辑验证。

## 4. 资源元数据库最佳实践

问题现象：
- 运行游戏后 `assets/metadata.json` 频繁出现脏改动。

根因：
- 运行时启动即扫描并回写资产元数据库，不适合作为普通游戏运行默认行为。

当前策略：
- 引擎新增 `EngineSpecification::RefreshAssetDatabaseOnStartup`。
- 默认 `false`（游戏/运行时只读，不改写）。
- 编辑器可显式设为 `true`（需要资源刷新时再启用）。

## 5. 依赖目录（.dependencies）注意事项

当前仓库以 gitlink 记录第三方依赖源码目录状态。常见坑：
- 本地依赖源码被更新后会显示工作区脏。
- 不应将本地临时依赖变更直接混入业务提交。

建议流程：
1. 业务改动完成后先检查 `git status`。
2. 如仅依赖目录指针变化，先确认是否为有意升级依赖。
3. 若非有意升级，恢复依赖指针再提交业务代码。

## 6. 常见编译/链接错误排查顺序

1. 先确认 preset 与目标是否匹配（平台/架构/target/config）。
2. 只构建目标游戏验证最小闭环：
   - `cmake --build --preset <pacman-preset> --target PacManGame`
3. 检查运行目录下 `assets` 是否已复制。
4. 若图形初始化失败，确认是否进入 console fallback。
5. 最后再回查引擎模块链接与符号可见性问题。

## 7. 游戏项目接入清单

新游戏项目应至少满足：
- 独立 CMake target（可单独开关）
- 独立 preset 与输出目录
- 资源复制到可执行目录
- 无图形设备可运行的 fallback view（可选但强烈推荐）
- README 中明确启动、构建、打包命令

