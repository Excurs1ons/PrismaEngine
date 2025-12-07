# YAGE 渲染系统使用指南

## 概述

YAGE引擎的渲染系统已经更新，现在支持从场景获取主相机、获取场景物体（三角形）和渲染组件并进行渲染。

## 主要组件

### 1. Scene 类

Scene类是场景管理的核心，负责管理所有游戏对象和主相机。

#### 主要方法：
- `AddGameObject(std::shared_ptr<GameObject> gameObject)`: 添加游戏对象到场景
- `RemoveGameObject(GameObject* gameObject)`: 从场景中移除游戏对象
- `Update(float deltaTime)`: 更新场景中的所有对象
- `Render(IRenderCommandContext* context)`: 渲染场景中的所有对象
- `GetGameObjects()`: 获取场景中的所有游戏对象
- `GetMainCamera()`: 获取主相机
- `SetMainCamera(ICamera* camera)`: 设置主相机

### 2. RenderComponent 类

RenderComponent是用于渲染几何体的组件，可以附加到游戏对象上。

#### 主要方法：
- `SetVertexData(const float* vertices, uint32_t vertexCount)`: 设置顶点数据
- `SetIndexData(const uint32_t* indices, uint32_t indexCount)`: 设置索引数据
- `Render(IRenderCommandContext* context)`: 渲染几何体
- `SetColor(float r, float g, float b, float a = 1.0f)`: 设置颜色

### 3. Camera2D 类

Camera2D是2D相机实现，继承自ICamera接口。

#### 主要方法：
- `SetPosition(float x, float y, float z = 0.0f)`: 设置相机位置
- `SetRotation(float rotation)`: 设置相机旋转
- `SetOrthographicProjection(...)`: 设置正交投影参数
- `GetViewMatrix()`: 获取视图矩阵
- `GetProjectionMatrix()`: 获取投影矩阵
- `SetClearColor(float r, float g, float b, float a = 1.0f)`: 设置清除颜色

## 使用示例

### 创建基本场景

```cpp
// 创建场景
auto scene = std::make_shared<Scene>();

// 创建相机
auto cameraObj = std::make_shared<GameObject>("MainCamera", std::make_unique<Transform>());
auto camera = cameraObj->AddComponent<Camera2D>();
camera->SetPosition(0.0f, 0.0f, 0.0f);
camera->SetOrthographicProjection(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 1000.0f);
camera->SetClearColor(0.1f, 0.2f, 0.3f, 1.0f);

// 添加相机到场景并设置为主相机
scene->AddGameObject(cameraObj);
scene->SetMainCamera(camera);

// 创建三角形
auto triangle = std::make_shared<GameObject>("Triangle", std::make_unique<Transform>());
auto renderComponent = triangle->AddComponent<RenderComponent>();

// 定义三角形顶点数据 (位置 + 颜色)
float triangleVertices[] = {
    // 位置 (x, y, z)     颜色 (r, g, b, a)
    0.0f, 0.25f, 0.0f,  1.0f, 0.0f, 0.0f, 1.0f,  // 顶点1
    0.25f, -0.25f, 0.0f,  1.0f, 0.0f, 0.0f, 1.0f,  // 顶点2
    -0.25f, -0.25f, 0.0f,  1.0f, 0.0f, 0.0f, 1.0f   // 顶点3
};

// 设置顶点数据
renderComponent->SetVertexData(triangleVertices, 3);

// 添加三角形到场景
scene->AddGameObject(triangle);
```

### 使用TriangleExample

为了方便使用，我们提供了一个TriangleExample类，可以快速创建包含相机和三角形的示例场景：

```cpp
// 创建示例
TriangleExample example;
auto scene = example.CreateExampleScene();

// 将场景设置到应用程序
ApplicationWindows::GetInstance().SetScene(scene);
```

## 渲染流程

1. 渲染器在BeginFrame()中从场景获取主相机
2. 从主相机获取清除颜色并设置渲染目标
3. 场景的Render()方法被调用，传入渲染命令上下文
4. 场景设置主相机的视图和投影矩阵到渲染上下文
5. 场景遍历所有游戏对象，查找RenderComponent并调用其Render()方法
6. RenderComponent设置自己的变换矩阵和颜色，然后发出绘制命令

## 注意事项

1. 确保为场景设置主相机，否则将使用默认的清除颜色
2. 渲染组件需要设置顶点数据才能正确渲染
3. 顶点数据格式为：位置(x,y,z) + 颜色(r,g,b,a)，每个顶点7个float
4. 渲染器会自动处理视图和投影矩阵的设置，渲染组件只需设置世界矩阵

## 扩展

系统设计为可扩展的，你可以：
- 创建自定义的渲染组件继承自RenderComponent
- 实现更复杂的几何体渲染
- 添加材质和纹理支持
- 实现光照和阴影效果