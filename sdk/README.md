# PrismaEngine SDK

欢迎使用 PrismaEngine SDK！这个目录包含了使用 PrismaEngine 开发游戏所需的所有文件。

## 目录结构

```
sdk/
├── include/PrismaEngine/    # 公共 C++ 头文件 (190+ 文件)
├── lib/                      # 预编译库 — 仅从 GitHub Release 获取
│   ├── linux/                # 仓库中为空, 下载 Release 后填充
│   ├── windows/
│   └── android/
├── samples/                 # 示例项目
│   ├── BasicTriangle/       # 最小的可运行示例
│   ├── BlockGame/           # 方块游戏示例
│   └── PrismaCraftStarter/  # Minecraft 风格游戏启动器
├── cmake/                   # CMake 配置文件 (find_package 就绪)
└── docs/                    # API 参考和迁移指南
```

## 快速开始

### 方式 A: 下载预编译 SDK (推荐, 无需引擎源码)

从 [GitHub Releases](https://github.com/Excurs1ons/PrismaEngine/releases) 下载对应平台的 SDK 压缩包:

```bash
# 1. 下载并解压
tar xzf PrismaEngine-SDK-0.1.0-linux.tar.gz

# 2. 构建示例项目
cd PrismaEngine-SDK-0.1.0-linux/samples/BasicTriangle
cmake -B build -DPrismaEngine_DIR=../..
cmake --build build
./build/BasicTriangle
```

### 方式 B: 使用仓库内的 SDK (本地开发)

如果你已克隆完整仓库:

```bash
# 仓库内的 sdk/ 目录已包含头文件和 cmake 配置
cd sdk/samples/BasicTriangle
cmake -B build -DPrismaEngine_DIR=$(pwd)/../..
cmake --build build
./build/BasicTriangle
```

### 创建新项目

```bash
mkdir MyGame && cd MyGame

# CMakeLists.txt
cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.20)
project(MyGame VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PrismaEngine REQUIRED)

add_executable(MyGame src/main.cpp)
target_link_libraries(MyGame PRIVATE PrismaEngine::Engine)
target_include_directories(MyGame PRIVATE ${PRISMAENGINE_INCLUDE_DIR})
EOF

# src/main.cpp
mkdir src
cat > src/main.cpp << 'EOF'
#include <PrismaEngine/PrismaEngine.h>

using namespace Prisma;

class MyGame : public Application {
public:
    MyGame() : Application({.Name = "MyGame"}) {}

    int OnInitialize() override {
        LOG_INFO("Game", "游戏初始化");
        return 0;
    }

    void OnUpdate(Timestep ts) override {
        // 每帧更新
    }

    void OnShutdown() override {
        LOG_INFO("Game", "游戏关闭");
    }
};

Prisma::Application* Prisma::CreateApplication() {
    return new MyGame();
}
EOF

# 构建
cmake -B build -DPrismaEngine_DIR=/path/to/sdk
cmake --build build
```

## 示例项目

### BasicTriangle

最小的可运行 PrismaEngine 应用。

**构建:**
```bash
cd samples/BasicTriangle
cmake -B build -DPrismaEngine_DIR=/path/to/sdk
cmake --build build
./build/BasicTriangle
```

### BlockGame

展示基础方塊游戏循环、输入处理、世界生成。

### PrismaCraftStarter

集成 PrismaEngine + PrismaCraft 的 Minecraft 风格游戏启动器。

## CMake 函数

SDK 提供以下 CMake 辅助函数:

### prisma_create_app

```cmake
prisma_create_app(MyGame
    FOLDER output
    SOURCES src/main.cpp
    LIBRARIES extra_library
)
```

### prisma_create_editor_extension

```cmake
prisma_create_editor_extension(MyExtension
    SOURCES src/Extension.cpp src/ExtensionPanel.cpp
)
```

## 平台支持

| 平台 | SDK 状态 | 备注 |
|------|---------|------|
| Linux x64 | ✅ Release 可用 | Ubuntu 22.04+ |
| Linux ARM64 | ✅ Release 可用 | 树莓派 / Termux |
| Windows x64 | ✅ Release 可用 | Windows 10+ |
| Android | ⏳ Release 待发布 | NDK r25+ |

## 系统要求

### 开发环境
- **CMake**: 3.20+
- **C++ 编译器**: GCC 11+ / Clang 13+ / MSVC 2026+
- **Vulkan SDK**: 1.3+ (如需 Vulkan 后端)

### 运行时
- **显卡**: Vulkan 1.3 或 OpenGL 4.5+
- **内存**: 4GB RAM
- **存储**: 500MB

## 文档

- [API 参考](docs/APIReference.md)
- [迁移指南](docs/MigrationGuide.md)

## 故障排除

### `find_package(PrismaEngine)` 失败

确保设置了 `PrismaEngine_DIR`:
```bash
cmake -B build -DPrismaEngine_DIR=/path/to/sdk
```

### 找不到头文件

SDK 的 `sdk/include/` 目录必须包含 `PrismaEngine/PrismaEngine.h`。如果缺失, 运行:
```bash
./scripts/sync-sdk-headers.sh
```

### 预编译库不存在

仓库内的 `sdk/lib/` 目录为空。请从 [GitHub Releases](https://github.com/Excurs1ons/PrismaEngine/releases) 下载。

## 发布新版本

```bash
# 1. 同步头文件
./scripts/sync-sdk-headers.sh

# 2. 打包 SDK (构建引擎 + 收集产物)
./scripts/package-sdk.sh 1.0.0 --platforms linux,windows

# 3. 上传 dist/*.tar.gz 到 GitHub Releases
#    https://github.com/Excurs1ons/PrismaEngine/releases/new
```

## 贡献

欢迎贡献！请查看主仓库的 [README.md](../README.md) 了解详情。

## 许可证

MIT License — 见 [LICENSE](../../LICENSE)

## 联系方式

- GitHub: https://github.com/Excurs1ons/PrismaEngine
- 问题反馈: https://github.com/Excurs1ons/PrismaEngine/issues
