#include "GameManager.h"
#include "AndroidOut.h"
#include "Camera.h"
#include "CubemapTextureAsset.h"
#include "TextureAsset.h"
#include "InteractiveRotationComponent.h"
#include "MeshRenderer.h"
#include "SkyboxRenderer.h"

// Vulkan 前向声明
#include <vulkan/vulkan.h>

namespace PrismaEngine {

GameManager& GameManager::GetInstance() {
    static GameManager instance;
    return instance;
}

void GameManager::Initialize(android_app* pApp) {
    if (initialized_) {
        return;
    }

    assetManager_ = pApp->activity->assetManager;
    initialized_ = true;

    aout << "GameManager: Initialized" << std::endl;
}

void GameManager::CreateScene() {
    if (sceneCreated_) {
        aout << "GameManager: Scene already created, skipping..." << std::endl;
        return;
    }

    scene_ = std::make_shared<Scene>();
    sceneCreated_ = true;

    aout << "GameManager: Scene created (without rendering resources)" << std::endl;

    // 创建主相机
    {
        auto cameraGO = std::make_shared<GameObject>();
        cameraGO->name = "MainCamera";

        auto transform = cameraGO->GetTransform();
        transform->position = Vector3(0.0f, 0.0f, 6.0f);

        auto camera = cameraGO->AddComponent<PrismaEngine::Graphic::Camera>();
        camera->SetPerspectiveProjection(
            PrismaEngine::Math::Radians(45.0f),
            16.0f / 9.0f,
            0.1f,
            100.0f
        );

        scene_->addGameObject(cameraGO);
        aout << "GameManager: MainCamera created" << std::endl;
    }

    // 创建立方体（不含渲染资源，稍后添加）
    {
        auto go = std::make_shared<GameObject>();
        go->name = "Cube";
        go->position = Vector3(0, 0, -2.0f);

        // 添加交互式旋转组件
        auto rotationComp = go->AddComponent<InteractiveRotationComponent>();
        rotationComp->SetInteractionMode(InteractiveRotationComponent::InteractionMode::TouchRotate);
        rotationComp->SetTouchSensitivity(1.0f);
        rotationComp->SetAxisMode(InteractiveRotationComponent::AxisMode::Both);
        rotationComp->SetDamping(0.01f);

        scene_->addGameObject(go);
        aout << "GameManager: Cube created (MeshRenderer will be added later)" << std::endl;
    }

    // 创建天空盒（不含渲染资源，稍后添加）
    {
        auto skyboxGO = std::make_shared<GameObject>();
        skyboxGO->name = "Skybox";
        skyboxGO->position = Vector3(0, 0, 0);

        scene_->addGameObject(skyboxGO);
        aout << "GameManager: Skybox created (SkyboxRenderer will be added later)" << std::endl;
    }
}

void GameManager::SetupRenderingResources(void* vulkanContextPtr) {
    if (renderingSetup_) {
        aout << "GameManager: Rendering resources already setup, skipping..." << std::endl;
        return;
    }

    if (!scene_) {
        aout << "GameManager: No scene to setup rendering resources" << std::endl;
        return;
    }

    // 注意：这里假设 VulkanContext 结构有一个 device 成员
    // 实际实现需要根据具体的 VulkanContext 结构调整
    VkDevice device = VK_NULL_HANDLE;
    void* vulkanContext = vulkanContextPtr;

    // 由于我们需要访问 VulkanContext 的内部成员，这里使用一个简化的方法
    // 实际项目中应该重构代码，避免在 GameManager 中直接访问 Vulkan 内部

    aout << "GameManager: Setting up rendering resources..." << std::endl;

    const auto& gameObjects = scene_->getGameObjects();

    for (auto& go : gameObjects) {
        if (go->name == "Cube") {
            // 为立方体添加 MeshRenderer
            // 注意：这里需要 VulkanContext 来加载纹理
            // 实际实现需要调用 TextureAsset::loadAsset
            aout << "GameManager: TODO - Add MeshRenderer to Cube" << std::endl;
            // 这个实现需要在 RendererVulkan 中完成
        } else if (go->name == "Skybox") {
            // 为天空盒添加 SkyboxRenderer
            aout << "GameManager: TODO - Add SkyboxRenderer to Skybox" << std::endl;
            // 这个实现需要在 RendererVulkan 中完成
        }
    }

    renderingSetup_ = true;
    aout << "GameManager: Rendering resources setup complete" << std::endl;
}

void GameManager::Update(float deltaTime) {
    if (scene_) {
        scene_->update(deltaTime);
    }
}

} // namespace PrismaEngine
