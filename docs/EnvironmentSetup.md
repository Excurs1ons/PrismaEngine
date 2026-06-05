# 开发环境配置指南 (Environment Setup Guide)

本指南旨在帮助开发者快速配置 Prisma Engine 的开发环境，特别是针对网络受限环境和特定操作系统的优化建议。

## 1. 网络环境优化 (Network Optimization)

Prisma Engine 使用 CMake `FetchContent` 自动管理依赖。在网络受限环境下，建议进行以下配置以提升下载速度和稳定性。

### 1.1 Git 代理配置
如果你在拉取依赖时遇到连接超时，可以配置 Git 全局代理：

```shell
# 配置全局 HTTP/HTTPS 代理 (示例端口 7897)
git config --global http.proxy 127.0.0.1:7897
git config --global https.proxy 127.0.0.1:7897

# 或者使用 socks5 代理
git config --global http.proxy socks5://127.0.0.1:7897
```

### 1.2 Gradle 镜像配置 (Android 开发)
在进行 Android 构建时，建议配置国内镜像仓库：

**修改 `gradle-wrapper.properties`:**
```properties
distributionUrl=https\://mirrors.cloud.tencent.com/gradle/gradle-8.12-bin.zip
```

**修改 `settings.gradle` 中的仓库配置:**
```gradle
pluginManagement {
    repositories {
        maven { url 'https://maven.aliyun.com/repository/gradle-plugin' }
        maven { url 'https://maven.aliyun.com/repository/public' }
        google()
        mavenCentral()
        gradlePluginPortal()
    }
}

dependencyResolutionManagement {
    repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS)
    repositories {
        maven { url 'https://maven.aliyun.com/repository/public' }
        maven { url 'https://maven.aliyun.com/repository/google' }
        maven { url 'https://maven.aliyun.com/repository/central' }
        google()
        mavenCentral()
    }
}
```

## 2. 操作系统特定优化 (OS Specific Setup)

### 2.1 Windows (中文环境) 编码优化
在中文 Windows 环境下，MSVC 和 MSBuild 默认使用 GBK 编码，常导致控制台输出乱码。

**推荐方案：** 在构建前强制设置环境变量使用英文（1033），这在 CI 和本地开发中最为稳妥。

```powershell
# 执行构建前的组合命令
$env:VSLANG = '1033'; $env:DOTNET_CLI_UI_LANGUAGE = 'en-US'; chcp 437; ./scripts/build-windows.bat
```

### 2.2 Linux 依赖安装
在 Ubuntu/Debian 系统上，确保安装了以下基础开发库：

```shell
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libx11-dev \
    libxrandr-dev \
    libvulkan-dev \
    libsdl3-dev \
    dotnet-sdk-10.0
```

## 3. 编译器与 IDE 建议

- **编译器：** 必须支持 C++23。推荐使用 MSVC 17.10+ (VS 2022), GCC 13+, 或 Clang 16+。
- **IDE：** 
    - **Visual Studio 2022:** 推荐安装 "使用 C++ 的桌面开发" 和 ".NET 桌面开发" 工作负载。
    - **CLion:** 完美支持 CMake Presets。
    - **VS Code:** 建议安装 `C/C++ Extension Pack`, `CMake Tools` 和 `C# Dev Kit`。

## 4. 故障排除 (Troubleshooting)

- **FetchContent 失败:** 检查 Git 代理或手动清理 `build/` 目录。
- **C# 脚本加载失败:** 确保已安装 .NET 10 SDK 并且执行了 `dotnet publish`。
- **Vulkan 初始化失败:** 检查显卡驱动是否支持 Vulkan 1.3，并确保安装了最新的 Vulkan SDK。
