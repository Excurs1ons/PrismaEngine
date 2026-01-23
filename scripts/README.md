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
# Using the unified build script (recommended)
./scripts/build.sh linux-x64-debug

# Or use the platform-specific script
./scripts/build-linux.sh linux-x64-release

# Clean build
./scripts/build.sh linux-x64-debug clean
```

### Android
```bash
# From Linux/macOS/Windows (with WSL)
./scripts/build.sh android-arm64-v8a-debug

# Or use the platform-specific script
./scripts/build-android.sh android-arm64-v8a-release
```

## Available Presets

### Windows Presets
| Preset | Description | Build Type | Library Type |
|--------|-------------|------------|--------------|
| `windows-x64-debug` | x64 Debug build | Debug | Shared DLL |
| `windows-x64-release` | x64 Release build | Release | Static |
| `windows-x86-debug` | x86 Debug build | Debug | Shared DLL |
| `windows-x86-release` | x86 Release build | Release | Static |

### Linux Presets
| Preset | Description | Build Type | Backend |
|--------|-------------|------------|---------|
| `linux-x64-debug` | x64 Debug build | Debug | Vulkan |
| `linux-x64-release` | x64 Release build | Release | Vulkan |
| `linux-x64-debug-opengl` | x64 Debug build | Debug | OpenGL |
| `linux-x64-release-opengl` | x64 Release build | Release | OpenGL |

### Android Presets
| Preset | Description | Build Type | ABI |
|--------|-------------|------------|-----|
| `android-arm64-v8a-debug` | ARM64 Debug build | Debug | arm64-v8a |
| `android-arm64-v8a-release` | ARM64 Release build | Release | arm64-v8a |

## Platform-Specific Scripts

### Windows
- **`build.bat`** - Unified entry point (auto-detects platform)
- **`build-windows.bat`** - Batch script for Windows builds
- **`build-windows.ps1`** - PowerShell script for Windows builds

### Linux
- **`build.sh`** - Unified entry point (auto-detects platform)
- **`build-linux.sh`** - Bash script for Linux builds

### Android
- **`build-android.sh`** - Bash script for Android builds (Linux/macOS/WSL)
- **`build-android.bat`** - Batch script for Android builds (Windows)
- **`build-android.ps1`** - PowerShell script for Android builds (Windows)

## Environment Setup

### Windows
1. Install Visual Studio 2022 with C++ development tools
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
Add `clean` as the second argument to remove the build directory before building:

```bash
# Linux/macOS
./scripts/build.sh linux-x64-debug clean

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

Build artifacts are placed in the `build/` directory with the following structure:

```
build/
├── windows-x64-debug/
├── windows-x64-release/
├── linux-x64-debug/
├── linux-x64-release/
└── android-arm64-v8a-debug/
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
  run: ./scripts/build.sh linux-x64-release
```

## Additional Scripts

- **`build-status.bat`** - Check build status
- **`watch-build.bat`** - Watch and rebuild on file changes
- **`monitor-build.ps1`** - Monitor build progress

## Related Documentation

- [CLAUDE.md](../CLAUDE.md) - Main documentation
- [CMakePresets.json](../CMakePresets.json) - CMake preset definitions
- [docs/Building.md](../docs/Building.md) - Detailed build instructions
