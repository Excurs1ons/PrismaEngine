#pragma once
#include "Camera.h"
#include "GameObject.h"
#include "Scene.h"
#include "Transform.h"
#include <memory>
using namespace PrismaEngine;
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
    std::shared_ptr<GameObject> CreateTriangle(const std::string& name, PrismaEngine::Vector3 pos, PrismaEngine::Vector4 color);

    // 创建四边形游戏对象（使用索引缓冲区）
    std::shared_ptr<GameObject> CreateQuad(const std::string& name, PrismaEngine::Vector3 pos, PrismaEngine::Vector4 color, float size);

    // 创建立方体游戏对象
    std::shared_ptr<GameObject> CreateCube(const std::string& name, PrismaEngine::Vector3 pos, PrismaEngine::Vector4 color, float size);

    // 创建地面游戏对象（平放）
    std::shared_ptr<GameObject> CreateGround(const std::string& name, PrismaEngine::Vector3 pos, PrismaEngine::Vector4 color, float size);

    // 创建相机游戏对象
    static std::shared_ptr<GameObject> CreateCamera(const std::string& name, PrismaEngine::Vector3 pos, Quaternion rotation);

    // 创建调试文本（FPS 等信息）
    std::shared_ptr<GameObject> CreateDebugText(const std::string& name);
};