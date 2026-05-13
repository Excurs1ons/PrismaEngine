#pragma once
#include "Camera.h"
#include "core/Node.h"
#include "graphic/RenderCommandContext.h"
#include "graphic/RenderComponent.h"
#include "graphic/ICamera.h"
#include "core/Timestep.h"
#include <memory>
#include <vector>
#include <string>

namespace Prisma {

class ENGINE_API Scene
{
public:
    Scene();
    ~Scene();

    // 创建并添加 Node 到场景
    Node CreateNode(const std::string& name = "Node");
    
    // 从场景中移除 Node
    void RemoveNode(Node node);
    
    // 更新场景中的所有 Node
    void Update(Timestep ts);
    
    // 场景名称与脏标记
    void SetName(const std::string& name) { m_Name = name; }
    const std::string& GetName() const { return m_Name; }
    void SetDirty(bool dirty) { m_IsDirty = dirty; }
    bool IsDirty() const { return m_IsDirty; }
      
    // 获取场景中的所有 Node
    const std::vector<Node>& GetNodes() const { return m_nodes; }

    // 兼容性接口：获取旧版 GameObject 列表（当前返回空）
    const std::vector<std::shared_ptr<class GameObject>>& GetGameObjects() const { 
        static std::vector<std::shared_ptr<class GameObject>> dummy;
        return dummy; 
    }

    // 兼容性接口
    void AddGameObject(std::shared_ptr<class GameObject> obj) { (void)obj; }
    void RemoveGameObject(std::shared_ptr<class GameObject> obj) { (void)obj; }
    
    // 获取主相机
    std::shared_ptr<Prisma::Graphic::ICamera> GetMainCamera();

    // 设置主相机 (非拥有引用)
    void SetMainCamera(std::shared_ptr<Prisma::Graphic::ICamera> camera);

    // 从 JSONC 文件加载场景
    bool Deserialize(const std::string& path);
    
    // 序列化场景到 JSON 文件
    bool Serialize(const std::string& path) const;

private:
    std::string m_Name = "Untitled";
    bool m_IsDirty = false;
    std::vector<Node> m_nodes;
    std::shared_ptr<Prisma::Graphic::ICamera> m_mainCamera;
};

} // namespace Prisma
