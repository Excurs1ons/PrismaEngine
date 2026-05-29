#pragma once

#include "Export.h"
#include "core/Timestep.h"
#include "math/MathTypes.h"
#include "Component.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <functional>

namespace Prisma {
namespace Graphic {

// 精灵动画帧
struct ENGINE_API AnimationFrame {
    Vector4 spriteRect;  // x, y, width, height
    float duration;      // 帧持续时间（秒）

    AnimationFrame() : spriteRect(0, 0, 1, 1), duration(0.1f) {}
    AnimationFrame(const Vector4& rect, float dur = 0.1f)
        : spriteRect(rect), duration(dur) {}
};

// 精灵动画
class ENGINE_API SpriteAnimation {
public:
    SpriteAnimation();
    ~SpriteAnimation() = default;

    void AddFrame(const AnimationFrame& frame) { m_frames.push_back(frame); }
    void ClearFrames() { m_frames.clear(); }
    size_t GetFrameCount() const { return m_frames.size(); }
    const std::vector<AnimationFrame>& GetFrames() const { return m_frames; }

    void Play() { m_isPlaying = true; m_isFinished = false; }
    void Pause() { m_isPlaying = false; }
    void Stop() { m_isPlaying = false; m_currentFrame = 0; m_frameTime = 0.0f; }
    bool IsPlaying() const { return m_isPlaying; }

    void SetLooping(bool loop) { m_looping = loop; }
    bool IsLooping() const { return m_looping; }

    void SetSpeed(float speed) { m_speed = speed; }
    float GetSpeed() const { return m_speed; }

    void Update(Timestep ts);

    const AnimationFrame& GetCurrentFrame() const;
    int GetCurrentFrameIndex() const { return m_currentFrame; }
    bool IsFinished() const { return m_isFinished; }

    void SetOnFrameChanged(std::function<void(int)> callback) { m_onFrameChanged = callback; }
    void SetOnAnimationFinished(std::function<void()> callback) { m_onAnimationFinished = callback; }

private:
    std::vector<AnimationFrame> m_frames;
    int m_currentFrame = 0;
    float m_frameTime = 0.0f;
    bool m_isPlaying = false;
    bool m_looping = true;
    bool m_isFinished = false;
    float m_speed = 1.0f;

    std::function<void(int)> m_onFrameChanged;
    std::function<void()> m_onAnimationFinished;
};

// 精灵动画组件
class ENGINE_API SpriteAnimationComponent : public Component {
public:
    SpriteAnimationComponent();
    virtual ~SpriteAnimationComponent() = default;
    ComponentId GetComponentId() const override { return GetComponentTypeId<SpriteAnimationComponent>(); }
    const char* GetComponentTypeName() const override { return "SpriteAnimationComponent"; }

    void AddAnimation(const std::string& animationName, std::shared_ptr<SpriteAnimation> animation) {
        m_animations[animationName] = animation;
    }

    std::shared_ptr<SpriteAnimation> GetAnimation(const std::string& animationName) const {
        auto it = m_animations.find(animationName);
        return it != m_animations.end() ? it->second : nullptr;
    }

    bool HasAnimation(const std::string& animationName) const { return m_animations.count(animationName) > 0; }

    void PlayAnimation(const std::string& animationName, bool restart = false);
    const std::string& GetCurrentAnimation() const { return m_currentAnimation; }

    virtual void Update(Timestep ts) override;

    const AnimationFrame& GetCurrentFrame() const;
    Vector4 GetCurrentSpriteRect() const;

private:
    std::unordered_map<std::string, std::shared_ptr<SpriteAnimation>> m_animations;
    std::string m_currentAnimation;
};

} // namespace Graphic
} // namespace Prisma
