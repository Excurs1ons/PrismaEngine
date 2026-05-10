#pragma once

#include "Application.h"
#include "graphic/OrthographicCamera.h"
#include "graphic/RenderSystem.h"
#include <memory>
#include <string>

namespace Prisma {

/**
 * @brief 2D 场景测试模板
 * 最小化 Renderer2D 使用示例，包含旋转彩色方块和文字渲染。
 * 作为新 2D 项目的起点模板。
 */
class Scene2DTestApp : public Application {
public:
    Scene2DTestApp();
    Scene2DTestApp(const ApplicationSpecification& spec);
    ~Scene2DTestApp() override = default;

    static ApplicationSpecification LoadSpecification(const std::string& filePath);

    void SetAutoQuit(bool quit) { m_autoQuit = quit; }

    // Application 接口
    int OnInitialize() override;
    void OnRender() override;
    void OnUpdate(Timestep ts) override;
    void OnEvent(Event& e) override;

private:
    bool LoadScene(const std::string& filePath);

    std::shared_ptr<Graphic::OrthographicCamera> m_camera;
    float m_totalTime = 0.0f;
    int m_frameCount = 0;
    float m_fpsTimer = 0.0f;
    float m_currentFps = 0.0f;

    // 测试场景元素
    struct TestSprite {
        Vector2 position;
        Vector2 size;
        Color color;
        float rotation;
        float rotationSpeed;
    };
    std::vector<TestSprite> m_sprites;

    bool m_autoQuit = false;

    // GPU 信息（仅在初始化时获取一次）
    std::string m_gpuName;
};

} // namespace Prisma
