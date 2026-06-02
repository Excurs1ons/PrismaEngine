#include "SpriteAnimation.h"

namespace Prisma {
namespace Graphic {

SpriteAnimation::SpriteAnimation() {}

void SpriteAnimation::Update(Timestep ts) {
    if (!m_isPlaying || m_frames.empty()) return;

    m_frameTime += static_cast<float>(ts) * m_speed;

    if (m_frameTime >= m_frames[m_currentFrame].duration) {
        m_frameTime = 0.0f;
        m_currentFrame++;

        if (m_currentFrame >= static_cast<int>(m_frames.size())) {
            if (m_looping) {
                m_currentFrame = 0;
            } else {
                m_currentFrame = static_cast<int>(m_frames.size()) - 1;
                m_isPlaying = false;
                m_isFinished = true;
                if (m_onAnimationFinished) m_onAnimationFinished();
            }
        }

        if (m_onFrameChanged) m_onFrameChanged(m_currentFrame);
    }
}

const AnimationFrame& SpriteAnimation::GetCurrentFrame() const {
    static AnimationFrame empty;
    if (m_frames.empty()) return empty;
    return m_frames[m_currentFrame];
}

SpriteAnimationComponent::SpriteAnimationComponent() {}

void SpriteAnimationComponent::PlayAnimation(const std::string& animationName, bool restart) {
    if (m_currentAnimation == animationName && !restart) return;

    auto anim = GetAnimation(animationName);
    if (anim) {
        if (auto current = GetAnimation(m_currentAnimation)) {
            current->Stop();
        }
        m_currentAnimation = animationName;
        anim->Play();
    }
}

void SpriteAnimationComponent::Update(Timestep ts) {
    auto anim = GetAnimation(m_currentAnimation);
    if (anim) {
        anim->Update(ts);
    }
}

const AnimationFrame& SpriteAnimationComponent::GetCurrentFrame() const {
    auto anim = GetAnimation(m_currentAnimation);
    if (anim) return anim->GetCurrentFrame();
    static AnimationFrame empty;
    return empty;
}

Vector4 SpriteAnimationComponent::GetCurrentSpriteRect() const {
    return GetCurrentFrame().spriteRect;
}

} // namespace Graphic
} // namespace Prisma
