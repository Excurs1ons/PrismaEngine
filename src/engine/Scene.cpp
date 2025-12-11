#include "Scene.h"
#include "Camera2D.h"
#include "Logger.h"
#include "MeshRenderer.h"
#include "RenderComponent.h"

Scene::Scene()
{
}

Scene::~Scene()
{
}

void Scene::AddGameObject(std::shared_ptr<GameObject> gameObject)
{
    m_gameObjects.push_back(gameObject);
}

void Scene::RemoveGameObject(GameObject* gameObject)
{
    m_gameObjects.erase(
        std::remove_if(m_gameObjects.begin(), m_gameObjects.end(),
            [gameObject](const std::shared_ptr<GameObject>& obj) {
                return obj.get() == gameObject;
            }),
        m_gameObjects.end()
    );
}

void Scene::Update(float deltaTime)
{
    for (auto& obj : m_gameObjects) {
        obj->Update(deltaTime);
    }
}

void Scene::Render(RenderCommandContext* context)
{
    // 首先设置主相机的视图和投影矩阵
    if (m_mainCamera) {
        // 设置视图矩阵
        XMMATRIX viewMatrix = static_cast<Camera2D*>(m_mainCamera)->GetViewMatrix();
        context->SetConstantBuffer("View", viewMatrix);
        
        // 设置投影矩阵
        XMMATRIX projMatrix = static_cast<Camera2D*>(m_mainCamera)->GetProjectionMatrix();
        context->SetConstantBuffer("Projection", projMatrix);
        
        // 设置清除颜色
        XMVECTOR clearColor = m_mainCamera->GetClearColor();
        float clearColorArray[4] = {
            XMVectorGetX(clearColor),
            XMVectorGetY(clearColor),
            XMVectorGetZ(clearColor),
            XMVectorGetW(clearColor)
        };
        context->SetConstantBuffer("ClearColor", clearColorArray, 4);
    }
    
    // 渲染所有游戏对象
    for (auto& obj : m_gameObjects) {
        // 首先尝试获取RenderComponent并渲染
        auto renderComponent = obj->GetComponent<RenderComponent>();
        if (renderComponent) {
            renderComponent->Render(context);
        }
        
        // 如果没有RenderComponent，则尝试获取MeshRenderer组件并渲染（保持向后兼容）
        auto meshRenderer = obj->GetComponent<MeshRenderer>();
        if (meshRenderer) {
            meshRenderer->Render(context);
        }
    }
}

const std::vector<std::shared_ptr<GameObject>>& Scene::GetGameObjects() const
{
    return m_gameObjects;
}

Camera* Scene::GetMainCamera()
{
    return m_mainCamera;
}

void Scene::SetMainCamera(Camera* camera)
{
    m_mainCamera = camera;
    LOG_INFO("Scene", "Main camera set to {0}", camera ? "valid camera" : "nullptr");
}