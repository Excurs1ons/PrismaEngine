#pragma once

#include "Application.h"
#include "graphic/SpriteRenderer.h"
#include <memory>
#include <vector>

namespace Prisma {

class Template2DApp : public Application {
public:
    Template2DApp();
    ~Template2DApp() override = default;

    void SetAutoQuit(bool quit) { m_autoQuit = quit; }

    int OnInitialize() override;
    void OnRender() override;
    void OnUpdate(Timestep ts) override;
    void OnEvent(Event& e) override;

private:
    struct TestSprite {
        Vector2 position;
        Vector2 size;
        Color color;
        float rotation;
        float rotationSpeed;
    };
    std::vector<TestSprite> m_sprites;
    std::vector<std::shared_ptr<Graphic::SpriteRenderer>> m_sceneSprites;
    bool m_autoQuit = false;
    float m_cameraMoveSpeed = 600.0f;
    bool m_moveUp = false, m_moveDown = false, m_moveLeft = false, m_moveRight = false;
};

} // namespace Prisma
