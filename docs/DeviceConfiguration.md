# Prisma Engine 设备配置指南

## 概述

Prisma Engine 支持多个音频和渲染设备，可以通过 CMake 选项在编译时选择启用的设备。

## 音频设备

### 可用设备

- **XAudio2** (Windows) - 高性能，低延迟音频
- **OpenAL** (跨平台) - 专业 3D 音频 API
- **SDL3 Audio** (跨平台) - 简单易用的音频 API

### 配置选项

```cmake
# CMake 命令行配置
cmake .. -DPRISMA_ENABLE_AUDIO_XAUDIO2=ON \
          -DPRISMA_ENABLE_AUDIO_OPENAL=OFF \
          -DPRISMA_ENABLE_AUDIO_SDL3=OFF

# 或者使用 CMake GUI
# 1. 打开 CMake GUI
# 2. 配置项目
# 3. 勾选需要的设备选项
# 4. 点击 Configure 和 Generate
```

### 默认配置

- **Windows**: 仅启用 XAudio2
- **其他平台**: 需要手动启用至少一个设备

## 渲染设备

### 可用设备

- **DirectX 12** (Windows) - 现代 Windows 图形 API
- **OpenGL** (跨平台) - 传统图形 API (4.6+)
- **Vulkan** (跨平台) - 新一代跨平台图形 API (1.3+)
- **Metal** (macOS/iOS) - Apple 平台原生图形 API
- **WebGPU** (Web) - Web 平台图形 API

### 配置选项

```cmake
# CMake 命令行配置
cmake .. -DPRISMA_ENABLE_RENDER_DX12=ON \
          -DPRISMA_ENABLE_RENDER_OPENGL=OFF \
          -DPRISMA_ENABLE_RENDER_VULKAN=OFF

# 启用多个设备（运行时选择）
cmake .. -DPRISMA_ENABLE_RENDER_DX12=ON \
          -DPRISMA_ENABLE_RENDER_VULKAN=ON
```

### 默认配置

- **Windows**: 仅启用 DirectX 12
- **macOS/iOS**: 需要手动启用（推荐 Metal）
- **Linux**: 需要手动启用（推荐 Vulkan 或 OpenGL）
- **Web**: 自动启用 WebGPU

## 高级功能

### 音频功能

```cmake
# 启用 3D 音频
cmake .. -DPRISMA_ENABLE_AUDIO_3D=ON

# 启用音频流式播放
cmake .. -DPRISMA_ENABLE_AUDIO_STREAMING=ON

# 启用音效处理 (EAX/EFX)
cmake .. -DPRISMA_ENABLE_AUDIO_EFFECTS=ON

# 启用 HRTF (双耳音频)
cmake .. -DPRISMA_ENABLE_AUDIO_HRTF=ON
```

### 渲染功能

```cmake
# 启用光线追踪
cmake .. -DPRISMA_ENABLE_RAYTRACING=ON

# 启用网格着色器
cmake .. -DPRISMA_ENABLE_MESH_SHADERS=ON

# 启用可变速率着色
cmake .. -DPRISMA_ENABLE_VARIABLE_RATE_SHADING=ON

# 启用无绑定资源
cmake .. -DPRISMA_ENABLE_BINDLESS_RESOURCES=ON
```

## 平台特定注意事项

### Windows

```cmake
# 推荐配置（最佳性能）
cmake .. -DPRISMA_ENABLE_AUDIO_XAUDIO2=ON \
          -DPRISMA_ENABLE_RENDER_DX12=ON \
          -DPRISMA_ENABLE_RAYTRACING=ON
```

### macOS

```cmake
# 使用 Metal（推荐）
cmake .. -DPRISMA_ENABLE_AUDIO_OPENAL=ON \
          -DPRISMA_ENABLE_RENDER_METAL=ON

# 或使用跨平台选项
cmake .. -DPRISMA_ENABLE_AUDIO_OPENAL=ON \
          -DPRISMA_ENABLE_RENDER_OPENGL=ON
```

### Linux

```cmake
# 使用 Vulkan（推荐）
cmake .. -DPRISMA_ENABLE_AUDIO_OPENAL=ON \
          -DPRISMA_ENABLE_RENDER_VULKAN=ON

# 或使用 OpenGL
cmake .. -DPRISMA_ENABLE_AUDIO_OPENAL=ON \
          -DPRISMA_ENABLE_RENDER_OPENGL=ON
```

### Android

```cmake
# Android 配置
cmake .. -DPRISMA_ENABLE_AUDIO_OPENAL=ON \
          -DPRISMA_ENABLE_RENDER_VULKAN=ON
```

### Web (Emscripten)

```cmake
# Web 配置
emcmake cmake .. -DPRISMA_ENABLE_AUDIO_SDL3=ON \
                  -DPRISMA_ENABLE_RENDER_WEBGPU=ON
```

## 运行时设备选择

如果编译时启用了多个设备，可以在运行时选择：

```cpp
// 音频
AudioDesc audioDesc;
audioDesc.backendType = AudioDeviceType::OpenAL; // 或 XAudio2, SDL3

// 渲染
RenderSystemDesc renderDesc;
renderDesc.backendType = RenderDeviceType::Vulkan; // 或 DirectX12, OpenGL
```

## 故障排除

### 找不到依赖

如果启用某个后端但找不到相应的库：

1. **Windows (XAudio2)**:
   - Windows SDK 已包含，无需额外安装

2. **OpenAL**:
   - Windows: 下载 [OpenAL Soft](https://openal-soft.org/)
   - Linux: `sudo apt install libopenal-dev`
   - macOS: `brew install openal-soft`

3. **SDL3**:
   - 从 [SDL官网](https://www.libsdl.org/) 下载开发版本

4. **Vulkan**:
   - 安装 [Vulkan SDK](https://vulkan.lunarg.com/)

### 平台不兼容错误

如果看到平台不兼容错误，检查：
- Windows 只能使用 XAudio2 和 DirectX12
- macOS/iOS 只能使用 Metal
- Web 只能使用 WebGPU

### 性能建议

1. **Windows**: 使用 XAudio2 + DirectX12 组合获得最佳性能
2. **跨平台开发**: 使用 OpenAL + Vulkan 作为统一方案
3. **移动设备**: 考虑使用专门的移动端后端优化

## 配置文件示例

创建 `CMakePresets.json` 快速配置：

```json
{
  "version": 3,
  "configurePresets": [
    {
      "name": "Windows-Release",
      "generator": "Visual Studio 17 2022",
      "binaryDir": "${sourceDir}/build/${presetName}",
      "cacheVariables": {
        "PRISMA_ENABLE_AUDIO_XAUDIO2": "ON",
        "PRISMA_ENABLE_RENDER_DX12": "ON",
        "PRISMA_ENABLE_RAYTRACING": "ON"
      }
    },
    {
      "name": "Linux-Release",
      "binaryDir": "${sourceDir}/build/${presetName}",
      "cacheVariables": {
        "PRISMA_ENABLE_AUDIO_OPENAL": "ON",
        "PRISMA_ENABLE_RENDER_VULKAN": "ON"
      }
    }
  ]
}
```