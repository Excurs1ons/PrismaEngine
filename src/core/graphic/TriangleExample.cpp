#include "TriangleExample.h"
#include "Logger.h"
#include "Material.h"
#include "ResourceManager.h"
#include "../CameraController.h"

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
    auto triangle1 = CreateTriangle("Triangle1", -0.7f, 0.0f, 1.0f, 0.0f, 0.0f); // 红色
    auto triangle2 = CreateTriangle("Triangle2", 0.7f, 0.0f, 0.0f, 1.0f, 0.0f); // 绿色

    // 创建一个四边形来测试索引缓冲区
    auto quad = CreateQuad("TestQuad", 0.0f, 0.0f, 0.3f, 0.0f, 0.0f, 1.0f); // 蓝色四边形

    // 添加更多参考对象来观察相机移动
    auto referenceQuad1 = CreateQuad("RefQuad1", -2.0f, 1.5f, 0.2f, 1.0f, 1.0f, 0.0f); // 黄色
    auto referenceQuad2 = CreateQuad("RefQuad2", 2.0f, -1.5f, 0.2f, 1.0f, 0.0f, 1.0f); // 品红色
    auto referenceTriangle1 = CreateTriangle("RefTri1", 0.0f, 2.0f, 1.0f, 0.5f, 0.5f); // 粉色
    auto referenceTriangle2 = CreateTriangle("RefTri2", 0.0f, -2.0f, 0.5f, 0.5f, 1.0f); // 浅蓝色

    // 添加到场景
    scene->AddGameObject(triangle1);
    scene->AddGameObject(triangle2);
    scene->AddGameObject(quad);
    scene->AddGameObject(referenceQuad1);
    scene->AddGameObject(referenceQuad2);
    scene->AddGameObject(referenceTriangle1);
    scene->AddGameObject(referenceTriangle2);
    
    LOG_INFO("TriangleExample", "示例场景创建完成：1个相机，2个三角形，1个四边形（索引缓冲区测试）");
    
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

    // 创建并设置材质
    auto material = Material::CreateDefault();
    material->SetBaseColor(r, g, b, a);
    // material->SetName(name + "_Material"); // Material类没有SetName方法，暂时注释
    renderComponent->SetMaterial(material);
    
    LOG_DEBUG("TriangleExample", "Created triangle '{0}' at position ({1}, {2}) with color ({3}, {4}, {5}, {6})", 
        name, posX, posY, r, g, b, a);
    
    return gameObject;
}

std::shared_ptr<GameObject> TriangleExample::CreateQuad(const std::string& name, float posX, float posY,
                                                       float size, float r, float g, float b, float a)
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

    // 定义四边形顶点数据 (位置 + 颜色) - 4个顶点
    float quadVertices[] = {
        // 位置 (x, y, z)              颜色 (r, g, b, a)
        posX - size/2, posY + size/2, 0.0f,  r, g, b, a,  // 左上
        posX + size/2, posY + size/2, 0.0f,  r, g, b, a,  // 右上
        posX + size/2, posY - size/2, 0.0f,  r, g, b, a,  // 右下
        posX - size/2, posY - size/2, 0.0f,  r, g, b, a   // 左下
    };

    // 定义索引数据 - 2个三角形，共6个索引
    uint16_t quadIndices[] = {
        0, 1, 2,  // 第一个三角形 (左上, 右上, 右下)
        0, 2, 3   // 第二个三角形 (左上, 右下, 左下)
    };

    // 设置顶点数据
    renderComponent->SetVertexData(quadVertices, 4);

    // 设置索引数据 - 这是新增的功能
    renderComponent->SetIndexData(quadIndices, 6);

    // 创建特殊材质 (具有不同的金属度和粗糙度)
    auto material = Material::CreateDefault();
    material->SetBaseColor(r, g, b, a);
    material->SetMetallic(0.8f);  // 高金属度
    material->SetRoughness(0.2f); // 低粗糙度 (更光滑)
    // material->SetName(name + "_MetallicMaterial"); // Material类没有SetName方法，暂时注释
    renderComponent->SetMaterial(material);

    LOG_DEBUG("TriangleExample", "创建四边形 '{0}' 在位置 ({1}, {2})，大小 {3}，颜色 ({4}, {5}, {6}, {7})",
        name, posX, posY, size, r, g, b, a);

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

    // 设置正交投影 - 使用基于宽高比的投影
    // 假设窗口大小，这里使用16:9的宽高比作为默认值
    float aspectRatio = 16.0f / 9.0f;
    float viewHeight = 2.0f;  // 视口高度为2个单位
    float viewWidth = viewHeight * aspectRatio;  // 根据宽高比计算宽度

    camera->SetOrthographicProjection(-viewWidth/2, viewWidth/2, -viewHeight/2, viewHeight/2, 0.1f, 1000.0f);

    // 设置清除颜色为深蓝色
    camera->SetClearColor(0.1f, 0.2f, 0.3f, 1.0f);

    // 添加相机控制器组件
    auto cameraController = gameObject->AddComponent<CameraController>();
    cameraController->SetMoveSpeed(2.0f);  // 设置移动速度
    
    LOG_DEBUG("TriangleExample", "Created camera '{0}' at position ({1}, {2})", name, posX, posY);
    
    return gameObject;
}
