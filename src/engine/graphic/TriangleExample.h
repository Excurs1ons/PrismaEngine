#pragma once
#include "Scene.h"
#include "GameObject.h"
#include "Transform.h"
#include "RenderComponent.h"
#include "Camera3D.h"
#include "Camera3DController.h"
#include <memory>

// 三角形示例类，展示如何创建场景、相机和渲染组件
class TriangleExample
{
public:
    TriangleExample();
    ~TriangleExample() = default;
    
    // 创建示例场景
    std::shared_ptr<Scene> CreateExampleScene();
    
private:
    // 创建三角形游戏对象
    std::shared_ptr<GameObject> CreateTriangle(const std::string& name, float posX, float posY, float posZ,
                                              float r, float g, float b, float a = 1.0f);

    // 创建四边形游戏对象（使用索引缓冲区）
    std::shared_ptr<GameObject> CreateQuad(const std::string& name, float posX, float posY, float posZ,
                                          float size, float r, float g, float b, float a = 1.0f);

    // 创建立方体游戏对象
    std::shared_ptr<GameObject> CreateCube(const std::string& name, float posX, float posY, float posZ,
                                          float size, float r, float g, float b, float a = 1.0f);

    // 创建地面游戏对象（平放）
    std::shared_ptr<GameObject> CreateGround(const std::string& name, float posX, float posY, float posZ,
                                            float size, float r, float g, float b, float a = 1.0f);

    // 创建相机游戏对象
    static std::shared_ptr<GameObject> CreateCamera(const std::string& name, XMFLOAT3 pos);
};