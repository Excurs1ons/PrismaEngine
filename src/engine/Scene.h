#pragma once
#include "Camera.h"
#include "GameObject.h"
#include "graphic/RenderCommandContext.h"
#include "graphic/RenderComponent.h"
#include "graphic/ICamera.h"
#include "SceneNode.h"
#include "core/Timestep.h"
#include <memory>
#include <vector>

namespace Prisma {

class ENGINE_API Scene
{
public:
    Scene();
    ~Scene();

    // 添加游戏对象到场景
    void AddGameObject(std::shared_ptr<GameObject> gameObject);
    
    // 从场景中移除游戏对象
    void RemoveGameObject(GameObject* gameObject);
    
    // 更新场景中的所有对象
    void Update(Timestep ts);
    
    // 场景名称与脏标记
    void SetName(const std::string& name) { m_Name = name; }
    const std::string& GetName() const { return m_Name; }
    void SetDirty(bool dirty) { m_IsDirty = dirty; }
    bool IsDirty() const { return m_IsDirty; }
      
    // 获取场景中的所有游戏对象
    const std::vector<std::shared_ptr<GameObject>>& GetGameObjects() const;
    
    // 获取主相机
    std::shared_ptr<Prisma::Graphic::ICamera> GetMainCamera();

    // 设置主相机 (非拥有引用)
    void SetMainCamera(std::shared_ptr<Prisma::Graphic::ICamera> camera);

private:
    std::string m_Name = "Untitled";
    bool m_IsDirty = false;
    std::vector<std::shared_ptr<GameObject>> m_gameObjects;
    std::shared_ptr<Prisma::Graphic::ICamera> m_mainCamera;
};

} // namespace Prisma
