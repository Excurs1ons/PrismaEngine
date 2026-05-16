# Prisma Engine Build Scripts

This directory contains build scripts for compiling Prisma Engine on different platforms.

## Quick Start

### Windows
```cmd
# Using the unified build script (recommended)
scripts\build.bat windows-x64-debug

# Or use the platform-specific script
scripts\build-windows.bat windows-x64-release

# Clean build
scripts\build.bat windows-x64-debug clean
```

### Linux/macOS
```bash
# Auto-detect platform+arch and build engine debug
./scripts/build.sh

# Build editor release
./scripts/build.sh --target editor --config release

# Clean and build with explicit preset
./scripts/build.sh --preset editor-linux-arm64-debug --clean
```

### Android
Use explicit Android preset:
```bash
./scripts/build.sh --preset engine-android-arm64-debug
```

## Available Presets

### Windows Presets
| Preset | Description | Build Type | Library Type |
|--------|-------------|------------|--------------|
| `engine-windows-x64-debug` | Engine x64 Debug build | Debug | Shared |
| `editor-windows-x64-debug` | Editor x64 Debug build | Debug | Shared |
| `launcher-windows-x64-debug` | Launcher x64 Debug build | Debug | Shared |
| `engine-windows-x64-release` | Engine x64 Release build | Release | Static |

### Linux Presets
| Preset | Description | Build Type | Backend |
|--------|-------------|------------|---------|
| `engine-linux-x64-debug` | Engine x64 Debug build | Debug | Vulkan |
| `engine-linux-arm64-debug` | Engine ARM64 Debug build | Debug | Vulkan |
| `editor-linux-x64-debug` | Editor x64 Debug build | Debug | Vulkan |
| `editor-linux-arm64-debug` | Editor ARM64 Debug build | Debug | Vulkan |

### Android Presets
| Preset | Description | Build Type | ABI |
|--------|-------------|------------|-----|
| `engine-android-arm64-debug` | Engine ARM64 Debug build | Debug | arm64-v8a |
| `engine-android-arm64-release` | Engine ARM64 Release build | Release | arm64-v8a |
| `launcher-android-arm64-debug` | Launcher ARM64 Debug build | Debug | arm64-v8a |

## `build.sh` 参数说明

- `--target <engine|editor|launcher>`: 目标模块，默认 `engine`
- `--config <debug|release>`: 构建类型，默认 `debug`
- `--preset <name>`: 直接指定 preset（跳过自动识别）
- `--clean`: 构建前清理输出目录
- `--jobs <N>`: 并行任务数

自动选择规则：

- `preset = <target>-<platform>-<arch>-<config>`
- 例如 Linux ARM64 + `engine` + `debug` => `engine-linux-arm64-debug`
- 脚本会打印：检测平台、架构、最终 preset、输出目录

## Platform-Specific Scripts

### Windows
- **`build.bat`** - Unified entry point (auto-detects platform)
- **`build-windows.bat`** - Batch script for Windows builds
- **`build-windows.ps1`** - PowerShell script for Windows builds

### Linux
- **`build.sh`** - Unified entry point (auto-detects platform + architecture)

### Android
- **`build-android.sh`** - Bash script for Android builds (Linux/macOS/WSL)
- **`build-android.bat`** - Batch script for Android builds (Windows)
- **`build-android.ps1`** - PowerShell script for Android builds (Windows)

## Environment Setup

### Windows
1. Install Visual Studio 2026 with C++ development tools
2. Install CMake 3.31+
3. (Optional) Install vcpkg for dependency management

### Linux
```bash
# Install dependencies
sudo apt update
sudo apt install cmake ninja-build build-essential

# For Vulkan backend
sudo apt install libvulkan-dev vulkan-tools

# For OpenGL backend
sudo apt install libgl1-mesa-dev libglu1-mesa-dev
```

### Android
1. Install Android NDK (r27 or later)
2. Set environment variable:
   ```bash
   export ANDROID_NDK_HOME=/path/to/android-ndk
   ```
3. Install CMake and Ninja:
   ```bash
   sudo apt install cmake ninja-build
   ```

## Advanced Options

### Clean Build
Use `--clean` to remove the selected build directory before building:

```bash
# Linux/macOS
./scripts/build.sh --target engine --config debug --clean

# Windows
scripts\build.bat windows-x64-debug clean
```

### Building with CMake Directly
If you prefer using CMake directly without the scripts:

```bash
# Configure
cmake --preset windows-x64-debug

# Build
cmake --build --preset windows-x64-debug
```

### Custom Build Options
You can modify build options by editing `CMakePresets.json` or passing additional CMake arguments:

```bash
cmake -B build/linux-x64-debug \
    -DCMAKE_BUILD_TYPE=Debug \
    -DPRISMA_ENABLE_RENDER_VULKAN=ON \
    -DPRISMA_ENABLE_AUDIO_SDL3=ON
```

## Output Directory

Build artifacts are placed in the preset `binaryDir` configured in `CMakePresets.json`, for example:

```
build/
├── engine-linux-arm64-debug/
├── editor-linux-arm64-debug/
├── engine-linux-x64-release/
└── engine-android-arm64-debug/
```

## Troubleshooting

### CMake not found
- **Windows**: Install CMake and add it to your PATH
- **Linux**: `sudo apt install cmake`

### Android NDK not found
Set the `ANDROID_NDK_HOME` environment variable:
```bash
export ANDROID_NDK_HOME=/path/to/ndk
```

### Vulkan headers missing
```bash
# Linux
sudo apt install libvulkan-dev vulkan-tools
```

### Build fails with "command not found"
Ensure you have the required build tools:
```bash
# Linux
sudo apt install build-essential ninja-build
```

## CI/CD Integration

These scripts are designed to work seamlessly with CI/CD pipelines:

```yaml
# Example GitHub Actions workflow
- name: Build Prisma Engine
  run: ./scripts/build.sh --target engine --config release
```

## Additional Scripts

- **`build-status.bat`** - Check build status
- **`watch-build.bat`** - Watch and rebuild on file changes
- **`monitor-build.ps1`** - Monitor build progress

## Related Documentation

- [CLAUDE.md](../CLAUDE.md) - Main documentation
- [CMakePresets.json](../CMakePresets.json) - CMake preset definitions
- [docs/Building.md](../docs/Building.md) - Detailed build instructions
