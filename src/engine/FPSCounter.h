#pragma once

#include "Component.h"
#include "ui/TextRendererComponent.h"
#include <string>
#include <time.h>

/// @brief FPS 计数器组件
/// 每秒更新一次显示的 FPS 值
class FPSCounter : public Component {
public:
    FPSCounter() = default;
    ~FPSCounter() override = default;

    void Initialize() override {
        m_textRenderer = gameObject()->GetComponent<PrismaEngine::TextRendererComponent>();
        if (m_textRenderer) {
            m_textRenderer->SetText("FPS: --");
            m_textRenderer->SetColor({0.0f, 1.0f, 0.0f, 1.0f}); // 绿色
        }
    }

    void Update(float deltaTime) override {
        m_accumulatedTime += deltaTime;
        m_frameCount++;

        // 每秒更新一次 FPS 显示
        if (m_accumulatedTime >= 1.0f) {
            int fps = m_frameCount;
            float frameTime = (m_accumulatedTime / m_frameCount) * 1000.0f; // 毫秒

            if (m_textRenderer) {
                char buffer[128];
                snprintf(buffer, sizeof(buffer),
                    "FPS: %d\nFrame Time: %.2f ms\nDeltaTime: %.3f s",
                    fps, frameTime, deltaTime);
                m_textRenderer->SetText(buffer);

                // 根据 FPS 改变颜色
                if (fps >= 60) {
                    m_textRenderer->SetColor({0.0f, 1.0f, 0.0f, 1.0f}); // 绿色
                } else if (fps >= 30) {
                    m_textRenderer->SetColor({1.0f, 1.0f, 0.0f, 1.0f}); // 黄色
                } else {
                    m_textRenderer->SetColor({1.0f, 0.0f, 0.0f, 1.0f}); // 红色
                }
            }

            m_frameCount = 0;
            m_accumulatedTime = 0.0f;
        }
    }

private:
    PrismaEngine::TextRendererComponent* m_textRenderer = nullptr;
    float m_accumulatedTime = 0.0f;
    int m_frameCount = 0;
};
