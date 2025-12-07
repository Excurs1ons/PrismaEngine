#include "TriangleExample.h"
#include "Logger.h"

TriangleExample::TriangleExample()
{
}

std::shared_ptr<Scene> TriangleExample::CreateExampleScene()
{
    // 创建场景
    auto scene = std::make_shared<Scene>();
    
    // 创建相机
    auto cameraObj = CreateCamera("MainCamera", 0.0f, 0.0f);
    scene->AddGameObject(cameraObj);
    
    // 获取相机组件并设置为场景的主相机
    auto camera = cameraObj->GetComponent<Camera2D>();
    if (camera) {
        scene->SetMainCamera(camera);
        LOG_INFO("TriangleExample", "Main camera set for scene");
    }
    
    // 创建几个三角形
    auto triangle1 = CreateTriangle("Triangle1", 0.0f, 0.0f, 1.0f, 0.0f, 0.0f); // 红色
    auto triangle2 = CreateTriangle("Triangle2", 0.5f, 0.0f, 0.0f, 1.0f, 0.0f); // 绿色
    auto triangle3 = CreateTriangle("Triangle3", -0.5f, 0.0f, 0.0f, 0.0f, 1.0f); // 蓝色
    
    // 添加三角形到场景
    scene->AddGameObject(triangle1);
    scene->AddGameObject(triangle2);
    scene->AddGameObject(triangle3);
    
    LOG_INFO("TriangleExample", "Example scene created with 1 camera and 3 triangles");
    
    return scene;
}

std::shared_ptr<GameObject> TriangleExample::CreateTriangle(const std::string& name, float posX, float posY, 
                                                          float r, float g, float b, float a)
{
    // 创建游戏对象
    auto gameObject = std::make_shared<GameObject>(name);
    
    // 添加变换组件并设置位置
    auto transform = gameObject->transform();
    transform->position[0] = posX;
    transform->position[1] = posY;
    transform->position[2] = 0.0f;
    
    // 添加渲染组件
    auto renderComponent = gameObject->AddComponent<RenderComponent>();
    
    // 定义三角形顶点数据 (位置 + 颜色)
    float triangleVertices[] = {
        // 位置 (x, y, z)     颜色 (r, g, b, a)
        posX, posY + 0.25f, 0.0f,  r, g, b, a,  // 顶点1
        posX + 0.25f, posY - 0.25f, 0.0f,  r, g, b, a,  // 顶点2
        posX - 0.25f, posY - 0.25f, 0.0f,  r, g, b, a   // 顶点3
    };
    
    // 设置顶点数据
    renderComponent->SetVertexData(triangleVertices, 3);
    
    // 设置颜色
    renderComponent->SetColor(r, g, b, a);
    
    LOG_DEBUG("TriangleExample", "Created triangle '{0}' at position ({1}, {2}) with color ({3}, {4}, {5}, {6})", 
        name, posX, posY, r, g, b, a);
    
    return gameObject;
}

std::shared_ptr<GameObject> TriangleExample::CreateCamera(const std::string& name, float posX, float posY)
{
    // 创建游戏对象
    auto gameObject = std::make_shared<GameObject>(name);
    
    // 添加变换组件并设置位置
    auto transform = gameObject->transform();
    transform->position[0] = posX;
    transform->position[1] = posY;
    transform->position[2] = 0.0f;
    
    // 添加相机组件
    auto camera = gameObject->AddComponent<Camera2D>();
    
    // 设置相机位置
    camera->SetPosition(posX, posY, 0.0f);
    
    // 设置正交投影
    camera->SetOrthographicProjection(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 1000.0f);
    
    // 设置清除颜色为深蓝色
    camera->SetClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    
    LOG_DEBUG("TriangleExample", "Created camera '{0}' at position ({1}, {2})", name, posX, posY);
    
    return gameObject;
}
