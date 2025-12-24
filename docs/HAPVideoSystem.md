# HAP 视频播放系统

> **状态**: 🔲 规划中
> **优先级**: 低
> **依赖**: Snappy 压缩库集成完成

## 概述

HAP 是高性能 GPU 加速视频编解码格式，基于纹理压缩 (DXT/S3TC/BPTC)，CPU 开销极低。

## 功能特性

| 功能 | 说明 | 优先级 |
|------|------|--------|
| **HAP 解码** | 支持 HAP/Q/HAPAlpha/HAPQ | 高 |
| **纹理输出** | 解码结果直接输出到 GPU 纹理 | 高 |
| **播放控制** | Play/Pause/Seek/Loop | 高 |
| **音频同步** | 音视频同步播放 | 中 |
| **多轨道** | 支持多视频叠加 | 低 |
| **网络流** | 支持流式加载 | 低 |

## 系统架构

```
src/engine/video/
├── VideoPlayer.h/cpp          # 视频播放器核心
├── VideoDecoder.h/cpp         # 解码器接口
├── HAPDecoder.h/cpp           # HAP 格式解码器
├── VideoTexture.h/cpp         # 视频纹理输出
└── VideoComponent.h/cpp       # GameObject 组件

third_party/hap/
├── hap_decode/                # HAP 解码库
└── snappy/                    # Snappy 压缩库
```

## HAP 格式支持

| 格式 | 压缩比 | 质量 | Alpha |
|------|--------|------|-------|
| **HAP** | DXT4 | 中 | 否 |
| **HAP Alpha** | DXT5 + Alpha | 中 | 是 |
| **HAP Q** | BPTC | 高 | 否 |
| **HAP Q Alpha** | BPTC + Alpha | 高 | 是 |

## API 设计

```cpp
namespace Engine::Video {

class VideoPlayer {
public:
    void load(const std::string& path);
    void play();
    void pause();
    void stop();
    void seek(float timeSeconds);

    void setLoop(bool loop);

    std::shared_ptr<Texture> getOutputTexture() const;

    bool isPlaying() const;
    float getDuration() const;
    float getCurrentTime() const;
};

class VideoComponent : public Component {
public:
    void load(const std::string& path);
    void play();
    void pause();
    void stop();
};

} // namespace
```

## 使用示例

```cpp
// 创建视频播放组件
auto videoPlayer = gameObject->addComponent<VideoComponent>();
videoPlayer->load("assets/videos/intro.mov");
videoPlayer->setLoop(true);
videoPlayer->play();

// 输出到渲染纹理
auto outputTexture = videoPlayer->getOutputTexture();
material->setTexture("VideoTexture", outputTexture);
```

## 开发阶段

| 阶段 | 内容 | 状态 |
|------|------|------|
| Phase 1 | HAP 解码库集成 (hap-in-c) | ⏳ 计划中 |
| Phase 2 | VideoPlayer 核心实现 | ⏳ 计划中 |
| Phase 3 | GPU 纹理直接上传 | ⏳ 计划中 |
| Phase 4 | VideoComponent 组件 | ⏳ 计划中 |
| Phase 5 | 音频同步与播放列表 | ⏳ 计划中 |

## 参考资料

- [HAP Codecs](https://github.com/vidvox/hap)
- [HAP in C](https://github.com/vidvox/hap-in-c)
- [Snappy Compression](https://github.com/google/snappy)

---

*文档创建时间: 2025-12-25*
