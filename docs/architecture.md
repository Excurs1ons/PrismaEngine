# PrismaEngine 架构文档

## 概述

PrismaEngine 采用模块化设计，核心子系统（音频、输入）使用 Driver-Device 架构，实现平台解耦。

## 架构设计

### Driver-Device 模式

```
┌─────────────────────────────────────────────────────────────┐
│                      Application Layer                     │
│                  (Runtime, Game, Editor)                     │
└──────────────────────┬──────────────────────────────────────┘
                       │
┌──────────────────────▼──────────────────────────────────────┐
│                     Device Layer (高层抽象)                  │
│  ┌─────────────────┐  ┌─────────────────┐                  │
│  │   AudioDevice   │  │   InputDevice    │                  │
│  │  - 3D音频       │  │  - 输入映射       │                  │
│  │  - 音效处理     │  │  - 动作绑定       │                  │
│  │  - 资源管理     │  │  - 文本输入       │                  │
│  └────────┬────────┘  └────────┬────────┘                  │
└───────────┼──────────────────┼──────────────────────────────┘
            │                  │
┌───────────▼──────────────────▼──────────────────────────────┐
│                   Driver Layer (平台接口)                    │
│  ┌────────────────────────────────────────────────────┐    │
│  │              IAudioDriver / IInputDriver            │    │
│  │              (纯虚接口，定义契约)                      │    │
│  └────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
            │
┌───────────▼───────────────────────────────────────────────┐
│               Platform Drivers (平台实现)                  │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐                  │
│  │  Windows │  │  Android │  │  跨平台   │                  │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘                  │
│       │             │             │                          │
│  ┌────▼─────┐  ┌───▼──────┐  ┌───▼──────┐                  │
│  │ XAudio2  │  │  AAudio  │  │   SDL3   │                  │
│  │ RawInput │  │GameActy  │  │   SDL3   │                  │
│  │  XInput  │  │          │  │          │                  │
│  └─────────┘  └──────────┘  └──────────┘                  │
└─────────────────────────────────────────────────────────────┘
```

## 音频系统

### 目录结构

```
src/engine/audio/
├── core/
│   └── IAudioDriver.h          # 驱动接口
├── drivers/                     # 平台驱动实现
│   ├── AudioDriverXAudio2.{h,cpp}     # Windows (XAudio2)
│   └── AudioDriverAAudio.{h,cpp}      # Android (AAudio)
└── AudioDevice.{h,cpp}          # 高层设备
```

### 接口定义

```cpp
// IAudioDriver - 驱动接口
class IAudioDriver {
    virtual AudioFormat Initialize(const AudioFormat& format) = 0;
    virtual void Shutdown() = 0;
    virtual SourceId CreateSource() = 0;
    virtual bool QueueBuffer(SourceId id, const AudioBuffer& buf) = 0;
    virtual bool Play(SourceId id, bool loop) = 0;
    virtual void Stop(SourceId id) = 0;
    virtual void SetVolume(SourceId id, float volume) = 0;
    // ...
};
```

### 平台支持

| 平台 | 驱动 | API |
|------|------|-----|
| Windows | `AudioDriverXAudio2` | XAudio2 (SDK) |
| Android | `AudioDriverAAudio` | AAudio (NDK, API 26+) |

### 使用示例

```cpp
#include "audio/AudioDevice.h"

using namespace PrismaEngine::Audio;

AudioDevice device;
device.Initialize({});

AudioClip clip = LoadAudio("sound.wav");
device.PlayClip(clip, {.volume = 0.8f});
```

## 输入系统

### 目录结构

```
src/engine/input/
├── core/
│   └── IInputDriver.h          # 驱动接口
├── drivers/                     # 平台驱动实现
│   ├── InputDriverWin32.{h,cpp}       # Windows (RawInput + XInput)
│   ├── InputDriverGameActivity.{h,cpp} # Android (GameActivity)
│   └── InputDriverSDL3.{h,cpp}         # 跨平台 (SDL3)
└── InputDevice.{h,cpp}          # 高层设备
```

### 接口定义

```cpp
// IInputDriver - 驱动接口
class IInputDriver {
    virtual bool IsKeyDown(KeyCode key) const = 0;
    virtual bool IsKeyJustPressed(KeyCode key) const = 0;
    virtual const MouseState& GetMouseState() const = 0;
    virtual bool IsGamepadConnected(uint32_t index) const = 0;
    virtual const GamepadState& GetGamepadState(uint32_t index) const = 0;
    // ...
};
```

### 平台支持

| 平台 | 驱动 | 键盘/鼠标 | 手柄 |
|------|------|-----------|------|
| Windows | `InputDriverWin32` | RawInput (SDK) | XInput (SDK) |
| Android | `InputDriverGameActivity` | GameActivity (NDK) | - |
| 跨平台 | `InputDriverSDL3` | SDL3 | SDL3 |

### 使用示例

```cpp
#include "input/InputDevice.h"

using namespace PrismaEngine::Input;

InputDevice input;
input.Initialize();

// 键盘查询
if (input.IsKeyJustPressed(KeyCode::Space)) {
    // 跳跃
}

// 动作映射
input.AddActionMapping("Jump", KeyCode::Space);
if (input.IsActionJustPressed("Jump")) {
    // 跳跃
}

// 手柄
if (input.IsGamepadConnected(0)) {
    float axisX = input.GetGamepadAxis(0, GamepadAxis::LeftX);
}
```

## 编译选项

### Native 模式

```bash
# Windows - 使用平台 SDK 原生 API
cmake -DPRISMA_USE_NATIVE_AUDIO=ON -DPRISMA_USE_NATIVE_INPUT=ON ..

# Android - 使用平台 SDK 原生 API
cmake -DPRISMA_USE_NATIVE_AUDIO=ON -DPRISMA_USE_NATIVE_INPUT=ON ..
```

### 跨平台模式 (SDL3)

```bash
cmake -DPRISMA_USE_NATIVE_AUDIO=OFF -DPRISMA_USE_NATIVE_INPUT=OFF ..
```

## 设计原则

1. **单一职责**: Driver 只处理平台 API 交互，Device 处理高级功能
2. **接口隔离**: 驱动接口简洁，只包含必要方法
3. **依赖倒置**: 高层依赖接口，不依赖具体实现
4. **开闭原则**: 添加新平台只需实现驱动接口

## TODO

- [ ] 添加 Linux 音频驱动 (ALSA)
- [ ] 添加 Apple 音频驱动 (CoreAudio)
- [ ] 清理旧的 Backend/Device 文件
- [ ] 添加单元测试
