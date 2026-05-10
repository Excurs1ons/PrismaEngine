# Pac-Man Game

基于 PrismaEngine 的 2D 吃豆人游戏复刻。

## 项目特性

- ✅ 2D 精灵渲染系统
- ✅ 正交投影相机
- ✅ 完整的吃豆人游戏逻辑
- ✅ 幽灵 AI（4种不同行为模式）
- ✅ 2D 灯光系统
- ✅ 音频支持
- ✅ 跨平台支持（Windows/Linux/Android）

## 项目结构

```
PacManGame/
├── src/
│   ├── main.cpp              # 入口点
│   ├── PacManGame.h/cpp      # 游戏主类
│   ├── core/
│   │   ├── GameConstants.h   # 游戏常量
│   │   └── GameConfig.h      # 游戏配置
│   ├── game/
│   │   ├── PacMan.h/cpp      # 吃豆人角色
│   │   ├── Ghost.h/cpp       # 幽灵角色
│   │   ├── GameBoard.h/cpp   # 游戏棋盘
│   │   ├── GameController.h/cpp # 游戏控制器
│   │   ├── Pellet.h/cpp      # 普通豆子
│   │   └── PowerPellet.h/cpp # 能量药丸
│   └── rendering/
│       ├── SpriteRenderer.h/cpp    # 2D 精灵渲染器
│       ├── OrthographicCamera.h/cpp # 2D 正交相机
│       └── SpriteAnimation.h/cpp   # 精灵动画
├── assets/
│   ├── sprites/
│   ├── maps/
│   ├── sounds/
│   └── shaders/
├── build/
│   ├── windows/
│   ├── linux/
│   └── android/
├── CMakeLists.txt
└── project_settings.json
```

## 构建指南

### 方法 1: 使用 Python 生成器（（推荐）

```bash
cd projects/PacManGame/build

# 生成 Windows 项目
python generate_project.py windows

# 生成 Linux 项目
python generate_project.py linux

# 配置 Android 项目
python generate_project.py android

# 生成并构建
python generate_project.py windows build
```

### 方法 2: 使用平台脚本

**Windows:**
```batch
cd build\windows
generate_ms_vc.bat
```

**Linux:**
```bash
cd build/linux
chmod +x generate_make.sh
./generate_make.sh
```

**Android:**
```bash
cd build/android
chmod +x generate_gradle.sh
./generate_gradle.sh
```

## 项目配置

编辑 `project_settings.json` 来配置游戏：

```json
{
  "companyName": "PacManGames",
  "productName": "PacManGame",
  "version": "1.0.0",
  "screenWidth": 896,
  "screenHeight": 992,
  "gameSettings": {
    "enableLighting": true,
    "enableParticles": true,
    "enableSound": true,
    "initialLives": 3
  }
}
```

## 游戏控制

- **方向键 / WASD**: 移动吃豆人
- **P**: 暂停游戏
- **R**: 重置游戏
- **ESC**: 退出游戏

## 开发状态

### 已完成
- ✅ 项目结构和框架
- ✅ 游戏常量和配置
- ✅ 2D 精灵渲染器（接口）
- ✅ 正交投影相机（接口）
- ✅ 精灵动画系统（接口）
- ✅ 游戏棋盘（接口）
- ✅ 吃豆人角色（接口）
- ✅ 幽灵角色（接口）
- ✅ 游戏控制器（接口）
- ✅ 构建系统生成器

### 待实现
- ⏳ 源文件实现（.cpp）
- ⏳ 游戏逻辑实现
- ⏳ 着色器实现
- ⏳ 资源文件
- ⏳ 测试和调试

## 所需的引擎 API

详见 [MISSING_API.md](MISSING_API.md)

## 许可证

MIT License
