# PrismaEngine 游戏开发实战指南（基于真实踩坑）

本文档不是概念介绍，而是按“从 0 到可玩”的工程落地流程整理，重点覆盖本次开发中实际遇到的问题与规避方式。

## 1. 目标与前提

当前 `prune` 分支建议的技术路线：

- 平台层：SDL3
- 渲染后端：Vulkan
- 音频后端：SDL3

开发一个可玩的游戏，最小闭环必须包含：

1. 能构建并链接通过
2. 能进入引擎主循环（不是初始化后直接退出）
3. 输入事件接通（方向/暂停/退出）
4. 基础玩法状态可更新（移动、碰撞、得分、关卡）

## 2. 一键构建（推荐）

使用统一脚本自动识别平台与架构，并自动选择 preset：

```bash
./scripts/build.sh
```

常用参数：

```bash
./scripts/build.sh --target engine --config debug
./scripts/build.sh --target editor --config release
./scripts/build.sh --preset editor-linux-arm64-debug --clean
```

自动选择规则：

- `preset = <target>-<platform>-<arch>-<config>`
- 例如：`engine-linux-arm64-debug`

脚本会输出：

- 检测到的平台
- 检测到的架构
- 最终选择的 preset
- 输出目录（`binaryDir`）

## 3. CMake / Preset 常见坑

### 坑 1：preset 名看起来是 arm64，但实际配置是 x64

原因通常是平台配置里架构被硬编码。  
建议做法：从 `CMAKE_SYSTEM_PROCESSOR` 自动识别并规范化为 `x64/arm64/...`。

### 坑 2：CMake 引用了不存在的源文件

典型报错：

- `Cannot find source file: xxx.cpp`
- `No SOURCES given to target`

处理步骤：

1. 全局搜索该文件引用（`rg -n "missing_file.cpp"`）
2. 修复 `CMakeLists.txt` 的源列表
3. 重新 configure + build

## 4. 游戏程序必须接入 Engine 主循环

很多“能编译但不能玩”的根因是：游戏代码只 `Initialize()`，没有真正进入 `Engine::Run(...)`。

正确模式：

1. 创建 `Application` 子类
2. 在 `OnUpdate/OnRender/OnEvent` 中驱动游戏控制器
3. 在 `Game::Run()` 中：
   - 初始化 `Engine`
   - `engine.Run(std::move(app))`
   - 结束后再 `Shutdown`

如果 `Run()` 里只有注释或空实现，程序会“秒进秒退”。

## 5. 事件与输入接入规范

### 5.1 在 `Application::OnEvent` 中分发事件

建议：

1. 先调用基类 `Application::OnEvent(e)`（让输入系统和 Layer 先处理）
2. 使用 `EventDispatcher` 分发：
   - `WindowResizeEvent`
   - `KeyPressedEvent`
   - `KeyReleasedEvent`
   - 鼠标事件（按需）

### 5.2 键位不要用魔法数字

不要再用 `0/1/2/3` 表示方向。  
使用 SDL 扫描码常量（引擎窗口事件目前传的是 `scancode`）：

- `SDL_SCANCODE_UP / DOWN / LEFT / RIGHT`
- `SDL_SCANCODE_W / A / S / D`
- `SDL_SCANCODE_P` 或 `SDL_SCANCODE_SPACE`（暂停）
- `SDL_SCANCODE_ESCAPE`（退出）

## 6. PacMan 项目当前状态与最小可玩定义

`PacManGame` 的编译与链接可以通过，但“可玩”不等于“可编译”。

验收建议：

1. 启动后窗口保持运行，不应立即退出
2. 方向键/WASD 能控制 PacMan
3. 可暂停/恢复
4. 碰撞与得分逻辑生效
5. 至少有基础状态反馈（日志或 UI）

## 7. 调试顺序（高效排错）

建议按以下顺序处理：

1. 先修 configure 错误（CMake 级）
2. 再修 compile 错误（源码级）
3. 再修 link 错误（目标依赖级）
4. 最后做运行时逻辑验证（输入/循环/状态）

命令建议：

```bash
# 全量 editor（覆盖 engine + launcher + projects）
./scripts/build.sh --target editor --config debug

# 单独验证 PacMan 目标
cmake --build --preset editor-linux-arm64-debug --target PacManGame -j4
```

## 8. 发布前检查清单

- [ ] `./scripts/build.sh` 在目标平台可直接使用
- [ ] 目标游戏链接通过
- [ ] 启动后不秒退
- [ ] 输入映射为真实扫描码/键码
- [ ] 关键玩法路径可走通（开始、暂停、结束/重开）
- [ ] 文档中的 preset 名称与 `CMakePresets.json` 一致

---

如需进一步标准化，下一步建议补一个“游戏模板项目”（最小 `Application + GameController + EventDispatcher` 样板），新项目直接复制即可。
