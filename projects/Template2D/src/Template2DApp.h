#pragma once

#include "Application.h"
#include "graphic/SpriteRenderer.h"
#include <memory>
#include <string>
#include <vector>

namespace Prisma {

class Template2DApp : public Application {
public:
    Template2DApp();
    Template2DApp(const ApplicationSpecification& spec);
    ~Template2DApp() override = default;

    static ApplicationSpecification LoadSpecification(const std::string& filePath);
    void SetAutoQuit(bool quit) { m_autoQuit = quit; }

    // Application 接口
    int OnInitialize() override;
    void OnRender() override;
    void OnUpdate(Timestep ts) override;
    void OnEvent(Event& e) override;

private:
    // 动态精灵（含 rotationSpeed，由 App 管理）
    struct TestSprite {
        Vector2 position;
        Vector2 size;
        Color color;
        float rotation;
        float rotationSpeed;
    };
    std::vector<TestSprite> m_sprites;

    // 场景精灵缓存（SceneManager 加载后的 SpriteRenderer 引用）
    std::vector<std::shared_ptr<Graphic::SpriteRenderer>> m_sceneSprites;

    bool m_autoQuit = false;
};

} // namespace Prisma
