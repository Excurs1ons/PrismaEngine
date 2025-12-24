# 音频系统设计

> **状态**: 🔲 规划中
> **优先级**: 中
> **依赖**: 平台抽象层完成

## 概述

PrismaEngine 的音频系统提供跨平台的音频播放能力，支持多种后端实现。

## 设计目标

- 跨平台支持 (Windows, Android, Linux)
- 多种音频格式 (OGG, MP3, WAV, FLAC)
- 3D 空间音频
- 音频流式加载
- 音效池管理

## 技术选型

| 组件 | Windows | Android / Linux |
|------|---------|----------------|
| 后端 | XAudio2 | SDL3 Audio / OpenAL |
| 解码器 | Media Foundation | libav / stb_vorbis |
| 空间音频 | X3DAudio | OpenAL 3D |

## API 设计

```cpp
namespace Engine::Audio {

class AudioClip;
class AudioSource;
class AudioManager;

// 音频剪辑
class AudioClip {
public:
    static std::shared_ptr<AudioClip> load(const std::string& path);
    static std::shared_ptr<AudioClip> loadFromMemory(std::span<const uint8_t> data);

    float getDuration() const;
    int getChannels() const;
    int getSampleRate() const;
};

// 音频源
class AudioSource {
public:
    void setClip(std::shared_ptr<AudioClip> clip);
    void play();
    void pause();
    void stop();

    void setLoop(bool loop);
    void setVolume(float volume);  // 0.0 - 1.0
    void setPitch(float pitch);    // 0.5 - 2.0

    void setPosition(Vector3 position);
    void setVelocity(Vector3 velocity);

    bool isPlaying() const;
};

// 音频管理器
class AudioManager {
public:
    static AudioManager& getInstance();

    void update();  // 每帧调用

    std::shared_ptr<AudioSource> createSource();
    void setListener(Vector3 position, Vector3 velocity, Quaternion rotation);
};

} // namespace
```

## 架构设计

```
┌─────────────────────────────────────────────┐
│              Game Layer                      │
│  AudioSource::play(), setVolume(), ...      │
├─────────────────────────────────────────────┤
│           Audio Manager Layer                │
│  - Source Pool Management                   │
│  - Listener Control                         │
│  - 3D Spatial Audio                         │
├─────────────────────────────────────────────┤
│            Backend Interface                 │
│  - IAudioBackend                            │
│  - IAudioDecoder                            │
├─────────────────────────────────────────────┤
│          Platform Backend                    │
│  - XAudio2Backend (Windows)                 │
│  - SDL3AudioBackend (Cross-platform)        │
│  - OpenALBackend (Optional)                 │
├─────────────────────────────────────────────┤
│            Audio Decoders                    │
│  - OGG Vorbis Decoder                       │
│  - MP3 Decoder                              │
│  - WAV Decoder                              │
└─────────────────────────────────────────────┘
```

## 开发计划

| 阶段 | 内容 | 状态 |
|------|------|------|
| Phase 1 | 后端接口设计 | ⏳ 计划中 |
| Phase 2 | XAudio2 后端实现 | ⏳ 计划中 |
| Phase 3 | SDL3 Audio 后端实现 | ⏳ 计划中 |
| Phase 4 | 解码器集成 | ⏳ 计划中 |
| Phase 5 | 3D 空间音频 | ⏳ 计划中 |

## 使用示例

```cpp
// 播放背景音乐
auto bgm = AudioClip::load("assets/audio/bgm.ogg");
auto bgmSource = AudioManager::getInstance().createSource();
bgmSource->setClip(bgm);
bgmSource->setLoop(true);
bgmSource->play();

// 播放音效
auto sfx = AudioClip::load("assets/audio/jump.wav");
auto sfxSource = AudioManager::getInstance().createSource();
sfxSource->setClip(sfx);
sfxSource->play();
```

## 参考资料

- [XAudio2 Documentation](https://docs.microsoft.com/en-us/windows/win32/xaudio2/)
- [SDL3 Audio](https://wiki.libsdl.org/SDL3/CategoryAudio)
- [OpenAL Programmer's Guide](https://www.openal.org/documentation/openal-programming-guide.html)

---

*文档创建时间: 2025-12-25*
