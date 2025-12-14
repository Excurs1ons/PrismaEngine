#include "TriangleExample.h"
#include "Logger.h"
#include "Material.h"
#include "ResourceManager.h"
#include "../Camera3DController.h"

TriangleExample::TriangleExample()
{
}

std::shared_ptr<Scene> TriangleExample::CreateExampleScene()
{
    // 创建场景
    auto scene = std::make_shared<Scene>();
    
    // 创建相机
    auto cameraObj = CreateCamera("MainCamera", {0.0f, 1.0f, -5.0f}, Quaternion::Identity);
    scene->AddGameObject(cameraObj);
    
    // 获取相机组件并设置为场景的主相机
    auto camera = cameraObj->GetComponent<Camera3D>();
    if (camera) {
        scene->SetMainCamera(camera);
        LOG_INFO("TriangleExample", "Main camera set for scene");
    }
    
    // 创建几个三角形
    auto triangle1 = CreateTriangle("Triangle1", {-0.7f, 0.0f, 1.0f}, {0.0f,1.0f, 0.0f,1.0f}); // 红色
    auto triangle2 = CreateTriangle("Triangle2", {0.7f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}); // 绿色

    // 创建一个四边形来测试索引缓冲区
    auto quad = CreateQuad("TestQuad", {0.0f, 0.0f, 0.3f}, {0.0f, 0.0f, 0.0f, 1.0f}, 0.3f); // 蓝色四边形

    // 创建一个立方体
    auto cube = CreateCube("ExampleCube", {0.0f, 0.5f, 0.8f}, {1.0f, 0.8f, 0.0f, 1.0f}, 0.5f); // 黄色立方体

    // 创建一个地面四边形
    auto ground = CreateGround("Ground", {0.0f, -0.5f, 0.0f}, {0.0f, 0.3f, 0.0f, 1.0f}, 0.3f); // 深绿色地面

    // 添加更多参考对象来观察相机移动
    auto referenceQuad1 = CreateQuad("RefQuad1", {-2.0f, 1.5f, 0.2f}, {1.0f, 1.0f, 0.0f, 1.0f},0.5f); // 黄色
    auto referenceQuad2 = CreateQuad("RefQuad2", {2.0f, -1.5f, 0.2f}, {1.0f, 0.0f, 1.0f,1.0f},1.0f); // 品红色
    auto referenceTriangle1 = CreateTriangle("RefTri1",{ 0.0f, 2.0f, 1.0f}, {0.5f, 0.5f, 1.0f, 1.0f}); // 粉色
    auto referenceTriangle2 = CreateTriangle("RefTri2",{ 0.0f, -2.0f, 1.0f}, {0.5f, 0.5f, 1.0f, 1.0f}); // 浅蓝色

    // 添加到场景
    scene->AddGameObject(triangle1);
    scene->AddGameObject(triangle2);
    scene->AddGameObject(quad);
    scene->AddGameObject(cube);
    scene->AddGameObject(ground);
    scene->AddGameObject(referenceQuad1);
    scene->AddGameObject(referenceQuad2);
    scene->AddGameObject(referenceTriangle1);
    scene->AddGameObject(referenceTriangle2);
    
    LOG_INFO("TriangleExample", "示例场景创建完成：1个相机，2个三角形，1个四边形，1个立方体，1个地面（索引缓冲区测试）");
    
    return scene;
}

std::shared_ptr<GameObject> TriangleExample::CreateTriangle(const std::string& name, XMFLOAT3 pos,
                                                          XMFLOAT4 color)
{
    // 创建游戏对象
    auto gameObject = std::make_shared<GameObject>(name);
    
    // 添加变换组件并设置位置
    auto transform = gameObject->transform();
    transform->position.x = pos.x;
    transform->position.y = pos.y;
    transform->position.z = pos.z;
    
    // 添加渲染组件
    auto renderComponent = gameObject->AddComponent<RenderComponent>();

    // 定义三角形顶点数据 (位置 + 颜色)
    float triangleVertices[] = {
        // 位置 (x, y, z)     颜色 (r, g, b, a)
        pos.x, pos.y + 0.25f, 0.0f,  color.x, color.y, color.z, color.w,  // 顶点1
        pos.x + 0.25f, pos.y - 0.25f, 0.0f,  color.x, color.y, color.z, color.w,  // 顶点2
        pos.x - 0.25f, pos.y - 0.25f, 0.0f,  color.x, color.y, color.z, color.w   // 顶点3
    };

    // 设置顶点数据
    renderComponent->SetVertexData(triangleVertices, 3);

    // 创建并设置材质
    auto material = Material::CreateDefault();
    material->SetBaseColor(color.x, color.y, color.z, color.w);
    // material->SetName(name + "_Material"); // Material类没有SetName方法，暂时注释
    renderComponent->SetMaterial(material);
    
    LOG_DEBUG("TriangleExample", "Created triangle '{0}' at position ({1}, {2}) with color ({3}, {4}, {5}, {6})", 
        name, pos.x, pos.y, color.x, color.y, color.z, color.w);
    
    return gameObject;
}

std::shared_ptr<GameObject> TriangleExample::CreateQuad(const std::string& name, XMFLOAT3 pos,XMFLOAT4 color,float size)
{
    // 创建游戏对象
    auto gameObject = std::make_shared<GameObject>(name);

    // 添加变换组件并设置位置
    auto transform = gameObject->transform();
    transform->position.x = pos.x;
    transform->position.y = pos.y;
    transform->position.z = pos.z;

    // 添加渲染组件
    auto renderComponent = gameObject->AddComponent<RenderComponent>();

    // 定义四边形顶点数据 (位置 + 颜色) - 4个顶点
    float quadVertices[] = {
        // 位置 (x, y, z)              颜色 (r, g, b, a)
        pos.x - size/2, pos.y + size/2, 0.0f,  color.x, color.y, color.z, color.w,  // 左上
        pos.x + size/2, pos.y + size/2, 0.0f,  color.x, color.y, color.z, color.w,  // 右上
        pos.x + size/2, pos.y - size/2, 0.0f,  color.x, color.y, color.z, color.w,  // 右下
        pos.x - size/2, pos.y - size/2, 0.0f,  color.x, color.y, color.z, color.w   // 左下
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
    material->SetBaseColor(color.x, color.y, color.z, color.w);
    material->SetMetallic(0.8f);  // 高金属度
    material->SetRoughness(0.2f); // 低粗糙度 (更光滑)
    // material->SetName(name + "_MetallicMaterial"); // Material类没有SetName方法，暂时注释
    renderComponent->SetMaterial(material);

    LOG_DEBUG("TriangleExample", "创建四边形 '{0}' 在位置 ({1}, {2})，大小 {3}，颜色 ({4}, {5}, {6}, {7})",
        name, pos.x, pos.y, size, color.x, color.y, color.z, color.w);

    return gameObject;
}

std::shared_ptr<GameObject> TriangleExample::CreateCube(const std::string& name, XMFLOAT3 pos,
                                                        XMFLOAT4 color, float size)
{
    // 创建游戏对象
    auto gameObject = std::make_shared<GameObject>(name);

    // 添加变换组件并设置位置
    auto transform = gameObject->transform();
    transform->position.x = pos.x;
    transform->position.y = pos.y;
    transform->position.z = pos.z;

    // 为立方体添加一些旋转使其看起来更有立体感
    transform->rotation.x = 45.0f;  // 绕X轴旋转45度
    transform->rotation.y = 45.0f;  // 绕Y轴旋转45度

    // 添加渲染组件
    auto renderComponent = gameObject->AddComponent<RenderComponent>();

    // 创建立方体的顶点数据 - 真正的立方体，使用世界坐标
    float halfSize = size / 2.0f;
    float cubeVertices[] = {
        // 位置 (x, y, z)              颜色 (r, g, b, a)
        // 立方体的8个顶点
        // 前面4个顶点 (Z = halfSize)
        pos.x - halfSize, pos.y + halfSize, halfSize,  color.x, color.y, color.z, color.w,  // 0 - 左上
        pos.x + halfSize, pos.y + halfSize, halfSize,  color.x, color.y, color.z, color.w,  // 1 - 右上
        pos.x + halfSize, pos.y - halfSize, halfSize,  color.x, color.y, color.z, color.w,  // 2 - 右下
        pos.x - halfSize, pos.y - halfSize, halfSize,  color.x, color.y, color.z, color.w,  // 3 - 左下

        // 后面4个顶点 (Z = -halfSize)
        pos.x - halfSize, pos.y + halfSize, -halfSize,  color.x * 0.8f, color.y * 0.8f, color.z * 0.8f, color.w,  // 4 - 左上
        pos.x + halfSize, pos.y + halfSize, -halfSize,  color.x * 0.8f, color.y * 0.8f, color.z * 0.8f, color.w,  // 5 - 右上
        pos.x + halfSize, pos.y - halfSize, -halfSize,  color.x * 0.8f, color.y * 0.8f, color.z * 0.8f, color.w,  // 6 - 右下
        pos.x - halfSize, pos.y - halfSize, -halfSize,  color.x * 0.8f, color.y * 0.8f, color.z * 0.8f, color.w   // 7 - 左下
    };

    // 定义索引数据 - 12个三角形（6个面 × 2个三角形）
    uint16_t cubeIndices[] = {
        // 前面 (Z = halfSize)
        0, 1, 2,  0, 2, 3,
        // 后面 (Z = -halfSize)
        4, 7, 6,  4, 6, 5,
        // 左面 (X = -halfSize)
        0, 3, 7,  0, 7, 4,
        // 右面 (X = +halfSize)
        1, 5, 6,  1, 6, 2,
        // 顶面 (Y = +halfSize)
        0, 4, 5,  0, 5, 1,
        // 底面 (Y = -halfSize)
        3, 2, 6,  3, 6, 7
    };

    // 设置顶点数据
    renderComponent->SetVertexData(cubeVertices, 12);

    // 设置索引数据
    renderComponent->SetIndexData(cubeIndices, 18);

    // 创建特殊材质
    auto material = Material::CreateDefault();
    material->SetBaseColor(color.x, color.y, color.z, color.w);
    material->SetMetallic(0.3f);   // 中等金属度
    material->SetRoughness(0.5f);  // 中等粗糙度
    renderComponent->SetMaterial(material);

    LOG_DEBUG("TriangleExample", "创建立方体 '{0}' 在位置 ({1}, {2})，大小 {3}，颜色 ({4}, {5}, {6}, {7})",
        name, pos.x, pos.y, size, color.x, color.y, color.z, color.w);

    return gameObject;
}

std::shared_ptr<GameObject> TriangleExample::CreateGround(const std::string& name, XMFLOAT3 pos,
                                                         XMFLOAT4 color, float size)
{
    // 创建游戏对象
    auto gameObject = std::make_shared<GameObject>(name);

    // 添加变换组件并设置位置
    auto transform = gameObject->transform();
    transform->position.x = pos.x;
    transform->position.y = pos.y;
    transform->position.z = 0.0f;

    // 将四边形旋转90度，使其平放在地上（绕X轴旋转）
    transform->rotation.x = 90.0f;  // 绕X轴旋转90度，使Z轴向上

    // 添加渲染组件
    auto renderComponent = gameObject->AddComponent<RenderComponent>();

    // 定义四边形顶点数据 (位置 + 颜色) - 4个顶点
    float groundVertices[] = {
        // 位置 (x, y, z)              颜色 (r, g, b, a)
        pos.x - size/2, pos.y - size/2, 0.0f,  color.x, color.y, color.z, color.w,  // 后下
        pos.x + size/2, pos.y - size/2, 0.0f,  color.x, color.y, color.z, color.w,  // 后上
        pos.x + size/2, pos.y + size/2, 0.0f,  color.x, color.y, color.z, color.w,  // 前上
        pos.x - size/2, pos.y + size/2, 0.0f,  color.x, color.y, color.z, color.w   // 前下
    };

    // 定义索引数据 - 2个三角形，共6个索引
    uint16_t groundIndices[] = {
        0, 1, 2,  // 第一个三角形
        0, 2, 3   // 第二个三角形
    };

    // 设置顶点数据
    renderComponent->SetVertexData(groundVertices, 4);

    // 设置索引数据
    renderComponent->SetIndexData(groundIndices, 6);

    // 创建特殊材质
    auto material = Material::CreateDefault();
    material->SetBaseColor(color.x, color.y, color.z, color.w);
    material->SetMetallic(0.1f);   // 低金属度
    material->SetRoughness(0.8f);  // 高粗糙度（更像地面）
    renderComponent->SetMaterial(material);

    LOG_DEBUG("TriangleExample", "创建地面 '{0}' 在位置 ({1}, {2})，大小 {3}，颜色 ({4}, {5}, {6}, {7})",
        name, pos.x, pos.y, size, color.x, color.y, color.z, color.w);

    return gameObject;
}

std::shared_ptr<GameObject> TriangleExample::CreateCamera(const std::string& name, XMFLOAT3 pos, Quaternion rotation)
{
    // 创建游戏对象
    auto game_object = std::make_shared<GameObject>(name);

    // 添加变换组件并设置位置
    auto *transform = game_object->transform();
    transform->position.x = pos.x;
    transform->position.y = pos.y;
    transform->position.z = pos.z;

    // 添加3D相机组件
    auto *camera = game_object->AddComponent<Camera3D>();

    // 设置相机位置
    camera->SetPosition(pos.x, pos.y, pos.z);

    // 设置透视投影
    float aspect_ratio = 16.0f / 9.0f;
    camera->SetPerspectiveProjection(XM_PIDIV4, aspect_ratio, 0.1F, 1000.0F);

    // 设置清除颜色为深蓝色
    camera->SetClearColor(0.0F, 0.2f, 0.0f, 1.0f);

    // 设置相机旋转，使其看向原点
    camera->LookAt(0.0f, 0.0f, 0.0f);

    // 添加3D相机控制器组件
    auto camera_controller = game_object->AddComponent<Camera3DController>();
    camera_controller->SetMoveSpeed(5.0f);      // 设置移动速度
    camera_controller->SetRotationSpeed(90.0f);  // 设置旋转速度

    LOG_DEBUG("TriangleExample", "Created 3D camera '{0}' at position ({1}, {2}, {3})", name, pos.x, pos.y, pos.z);

    return game_object;
}
