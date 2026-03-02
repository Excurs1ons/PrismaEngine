# PrismaEngine SDK

欢迎使用 PrismaEngine SDK！这个目录包含了使用 PrismaEngine 开发游戏所需的所有文件。

## 目录结构

```
sdk/
├── include/PrismaEngine/    # 公共头文件
├── lib/                      # 预编译库（由 package-sdk.sh 生成）
│   ├── linux/               # Linux 平台库
│   ├── windows/             # Windows 平台库
│   └── android/             # Android 平台库
├── samples/                 # 示例项目
│   ├── BasicTriangle/       # 最小的可运行示例
│   └── ...
├── cmake/                   # CMake 配置文件
└── docs/                    # 文档
```

## 快速开始

### 1. 从源码构建（开发模式）

如果你想使用 PrismaEngine 源码进行开发：

```bash
# 克隆仓库
git clone https://github.com/Excurs1ons/PrismaEngine.git
cd PrismaEngine

# 构建引擎
./scripts/build-linux.sh linux-x64-debug

# 构建示例项目
cd sdk/samples/BasicTriangle
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DPrismaEngine_DIR=../../../build/linux-x64-debug
cmake --build build
./build/BasicTriangle
```

### 2. 使用已打包的 SDK

如果你想使用预打包的 SDK：

```bash
# 1. 首先打包 SDK
./scripts/package-sdk.sh 0.1.0

# 2. 解压到目标目录
# （假设解压到 /opt/PrismaEngine-SDK）

# 3. 创建新项目
mkdir MyGame && cd MyGame
cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.20)
project(MyGame VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PrismaEngine REQUIRED)

add_executable(MyGame src/main.cpp)
target_link_libraries(MyGame PRIVATE PrismaEngine::Engine)
EOF

# 4. 创建源文件
mkdir src
cat > src/main.cpp << 'EOF'
#include <PrismaEngine/PrismaEngine.h>

using namespace PrismaEngine;

class MyGame : public IApplication<MyGame> {
public:
    bool Initialize() override {
        LOG_INFO("MyGame", "游戏初始化");
        return Platform::Initialize();
    }

    int Run() override {
        // 游戏主循环
        return 0;
    }

    void Shutdown() override {
        Platform::Shutdown();
    }
};

int main() {
    MyGame game;
    game.Initialize();
    return game.Run();
}
EOF

# 5. 构建项目
cmake -B build -DPrismaEngine_DIR=/opt/PrismaEngine-SDK
cmake --build build
./build/MyGame
```

## 示例项目

### BasicTriangle

最简单的 PrismaEngine 应用程序示例。

**构建：**
```bash
cd sdk/samples/BasicTriangle
cmake -B build -DPrismaEngine_DIR=../../../build/linux-x64-debug
cmake --build build
./build/BasicTriangle
```

## CMake 函数

SDK 提供了以下 CMake 辅助函数：

### prisma_create_app

创建一个 PrismaEngine 应用程序：

```cmake
prisma_create_app(MyGame
    FOLDER output
    SOURCES src/main.cpp
    LIBRARIES extra_library
)
```

参数：
- `FOLDER`: 可选，设置输出目录
- `SOURCES`: 可选，指定源文件
- `LIBRARIES`: 可选，链接额外的库

### prisma_create_editor_extension

创建编辑器扩展：

```cmake
prisma_create_editor_extension(MyExtension
    SOURCES
        src/Extension.cpp
        src/ExtensionPanel.cpp
)
```

## 平台支持

| 平台 | 支持状态 | 备注 |
|------|---------|------|
| Linux x64 | ✅ 完全支持 | 推荐 Ubuntu 22.04+ |
| Linux ARM64 | ✅ 完全支持 | 需要 ARM64 toolchain |
| Windows x64 | ✅ 完全支持 | 需要 Windows 10+ |
| Android | ✅ 完全支持 | 需要 NDK r25+ |

## 系统要求

### 开发环境

- **CMake**: 3.20 或更高版本
- **C++ 编译器**:
  - Linux: GCC 11+ 或 Clang 13+
  - Windows: MSVC 2022+
  - Android: NDK r25+
- **Vulkan SDK**: 1.3+ (如果使用 Vulkan 后端)
- **Python**: 3.8+ (用于构建脚本)

### 运行时要求

- **显卡**: 支持 Vulkan 1.3 或 OpenGL 4.5+
- **内存**: 至少 4GB RAM
- **存储**: 至少 500MB 可用空间

## 文档

- [API 参考](docs/APIReference.md)
- [快速入门](docs/QuickStart.md)
- [平台支持](docs/PlatformSupport.md)

## 故障排除

### Linux: 找不到 Vulkan 头文件

```bash
# Ubuntu/Debian
sudo apt install libvulkan-dev vulkan-tools

# Fedora
sudo dnf install vulkan-devel vulkan-tools
```

### Windows: 找不到 SDL3

确保使用 `PRISMA_USE_FETCHCONTENT=ON`，或者手动安装 SDL3：
```bash
vcpkg install sdl3:x64-windows
```

### 编译错误: C++20 特性不支持

确保编译器支持 C++20：
```bash
# 检查 GCC 版本
g++ --version  # 需要 11+

# 检查 Clang 版本
clang++ --version  # 需要 13+
```

## 贡献

欢迎贡献！请查看 [CONTRIBUTING.md](../../CONTRIBUTING.md) 了解详情。

## 许可证

本项目采用 MIT 许可证。请参阅 [LICENSE](../../LICENSE) 文件了解详情。

## 联系方式

- GitHub: https://github.com/Excurs1ons/PrismaEngine
- 问题反馈: https://github.com/Excurs1ons/PrismaEngine/issues

---

**Happy Game Development! 🎮**
