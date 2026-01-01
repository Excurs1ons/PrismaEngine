#pragma once
#include "Camera.h"
#include "GameObject.h"
#include "graphic/RenderCommandContext.h"
#include "graphic/RenderComponent.h"
#include "graphic/ICamera.h"
#include "SceneNode.h"
#include <memory>
#include <vector>

using namespace PrismaEngine;

class Scene
{
public:
    Scene();
    ~Scene();

    // 添加游戏对象到场景
    void AddGameObject(std::shared_ptr<GameObject> gameObject);
    
    // 从场景中移除游戏对象
    void RemoveGameObject(GameObject* gameObject);
    
    // 更新场景中的所有对象
    void Update(float deltaTime);
    
      
    // 获取场景中的所有游戏对象
    const std::vector<std::shared_ptr<GameObject>>& GetGameObjects() const;
    
    // 获取主相机
    PrismaEngine::Graphic::ICamera* GetMainCamera();

    // 设置主相机 (非拥有引用)
    void SetMainCamera(PrismaEngine::Graphic::ICamera* camera);

private:
    std::vector<std::shared_ptr<GameObject>> m_gameObjects;
    PrismaEngine::Graphic::ICamera* m_mainCamera = nullptr;
};