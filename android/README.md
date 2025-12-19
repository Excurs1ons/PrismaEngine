# PrismaEngine Android Build

此目录包含构建PrismaEngine为Android动态库（libEngine.so）的配置文件。

## 环境要求

- Android NDK r27 或更高版本
- CMake 3.31 或更高版本
- Ninja (推荐)

## 环境变量设置

```bash
# Windows
set ANDROID_NDK_HOME=C:\Android\Sdk\ndk\27.0.12077973
set ANDROID_HOME=C:\Android\Sdk

# Linux/macOS
export ANDROID_NDK_HOME=/path/to/android-ndk
export ANDROID_HOME=/path/to/android-sdk
```

## 构建方法

### 方法1：使用构建脚本（推荐）

#### Windows:
```cmd
# 构建默认架构 (arm64-v8a)
scripts\build-android.bat

# 构建指定架构
scripts\build-android.bat --abi arm64-v8a

# 构建所有架构
scripts\build-android.bat --all

# Debug构建
scripts\build-android.bat --type Debug

# 清理构建目录
scripts\build-android.bat --clean

# 查看帮助
scripts\build-android.bat --help
```

#### Linux/macOS:
```bash
# 给脚本执行权限
chmod +x scripts/build-android.sh

# 构建默认架构 (arm64-v8a)
./scripts/build-android.sh

# 构建指定架构
./scripts/build-android.sh --abi arm64-v8a

# 构建所有架构
./scripts/build-android.sh --all

# Debug构建
./scripts/build-android.sh --type Debug

# 清理构建目录
./scripts/build-android.sh --clean

# 查看帮助
./scripts/build-android.sh --help
```

### 方法2：使用CMake Presets

```bash
# 配置项目
cmake --preset android-arm64

# 构建
cmake --build --preset android-arm64

# 或者使用workflow一次性构建所有架构
cmake --workflow android-all
```

### 方法3：手动CMake命令

```bash
mkdir build-android && cd build-android

cmake ../android \
    -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=24 \
    -DANDROID_STL=c++_shared \
    -DCMAKE_BUILD_TYPE=Release \
    -G Ninja

cmake --build .
```

## 输出文件

构建成功后，libEngine.so将位于：
- `build-android/install/<abi>/lib/libEngine.so`

支持的ABI架构：
- `arm64-v8a` - ARM 64位（推荐）
- `armeabi-v7a` - ARM 32位
- `x86_64` - x86 64位
- `x86` - x86 32位

## 依赖说明

Android版本会自动排除Windows特定代码，并使用以下替代方案：
- 音频系统：使用SDL3音频后端（替代XAudio2）
- 平台层：使用SDL3平台抽象（替代Windows API）
- 渲染：保留Vulkan支持，DirectX仅在Windows版本中可用

## 故障排除

1. **NDK未找到**：确保设置了ANDROID_NDK_HOME或ANDROID_HOME环境变量
2. **编译错误**：检查CMake版本是否为3.31+
3. **链接错误**：确保安装了所需的Android系统库
4. **权限错误**：在Linux/macOS上给脚本添加执行权限

## 集成到Android项目

将生成的libEngine.so和头文件复制到Android项目的相应位置：

```
YourAndroidProject/
├── app/
│   ├── src/main/
│   │   ├── jniLibs/
│   │   │   ├── arm64-v8a/libEngine.so
│   │   │   ├── armeabi-v7a/libEngine.so
│   │   │   └── x86/libEngine.so
│   │   └── cpp/
│   │       └── include/
│   │           └── prisma/
│   │               └── engine/
│   │                   └── *.h
```