#pragma once
#include "Scene.h"
#include "GameObject.h"
#include "Transform.h"
#include "RenderComponent.h"
#include "Camera2D.h"
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
    std::shared_ptr<GameObject> CreateTriangle(const std::string& name, float posX, float posY,
                                              float r, float g, float b, float a = 1.0f);

    // 创建四边形游戏对象（使用索引缓冲区）
    std::shared_ptr<GameObject> CreateQuad(const std::string& name, float posX, float posY,
                                          float size, float r, float g, float b, float a = 1.0f);

    // 创建相机游戏对象
    std::shared_ptr<GameObject> CreateCamera(const std::string& name, float posX, float posY);
};