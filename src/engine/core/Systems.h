#pragma once

#include "ECS.h"
#include "Components.h"
#include <vector>

namespace PrismaEngine {
namespace Core {
namespace ECS {

// 渲染系统 - 处理MeshRenderer组件
class RenderSystem : public ISystem {
public:
    static constexpr SystemTypeID TYPE_ID = 1;

    SystemTypeID GetTypeID() const override { return TYPE_ID; }

    void Initialize() override;

    void Update(float deltaTime) override;

    void Shutdown() override;

private:
    // 收集所有需要渲染的实体
    void CollectRenderables();

    // 按材质排序以减少状态切换
    void SortByMaterial();

    // 渲染队列
    struct Renderable {
        EntityID entity;
        TransformComponent* transform;
        MeshRendererComponent* renderer;
        float distanceToCamera; // 用于透明物体排序
    };

    std::vector<Renderable> m_opaqueQueue;
    std::vector<Renderable> m_transparentQueue;

    // 主相机实体
    EntityID m_mainCamera = INVALID_ENTITY;
};

// 物理系统 - 处理RigidBody和Collider组件
class PhysicsSystem : public ISystem {
public:
    static constexpr SystemTypeID TYPE_ID = 2;

    SystemTypeID GetTypeID() const override { return TYPE_ID; }

    void Initialize() override;

    void Update(float deltaTime) override;

    void Shutdown() override;

    // 重力设置
    void SetGravity(const DirectX::XMFLOAT3& gravity);
    DirectX::XMFLOAT3 GetGravity() const { return m_gravity; }

private:
    DirectX::XMFLOAT3 m_gravity = DirectX::XMFLOAT3(0, -9.81f, 0);
    float m_fixedTimeStep = 1.0f / 60.0f;
    float m_accumulator = 0.0f;

    // 固定更新
    void FixedUpdate(float fixedDeltaTime);

    // 碰撞检测
    void DetectCollisions();

    // 碰撞响应
    void ResolveCollisions();
};

// 动画系统 - 处理Animation组件
class AnimationSystem : public ISystem {
public:
    static constexpr SystemTypeID TYPE_ID = 3;

    SystemTypeID GetTypeID() const override { return TYPE_ID; }

    void Initialize() override;

    void Update(float deltaTime) override;

    void Shutdown() override;

private:
    // 更新动画
    void UpdateAnimation(EntityID entity, AnimationComponent* anim, float deltaTime);
};

// 音频系统 - 处理AudioSource组件
class AudioSystem : public ISystem {
public:
    static constexpr SystemTypeID TYPE_ID = 4;

    SystemTypeID GetTypeID() const override { return TYPE_ID; }

    void Initialize() override;

    void Update(float deltaTime) override;

    void Shutdown() override;

    // 播放音频
    void PlayAudio(EntityID entity, const std::string& audioPath);

    // 停止音频
    void StopAudio(EntityID entity);

    // 设置3D音频位置
    void SetAudioPosition(EntityID entity, const DirectX::XMFLOAT3& position);

private:
    // 更新音频源
    void UpdateAudioSource(EntityID entity, AudioSourceComponent* audio);
};

// 脚本系统 - 处理Script组件
class ScriptSystem : public ISystem {
public:
    static constexpr SystemTypeID TYPE_ID = 5;

    SystemTypeID GetTypeID() const override { return TYPE_ID; }

    void Initialize() override;

    void Update(float deltaTime) override;

    void Shutdown() override;

    // 调用脚本函数
    void CallScriptFunction(EntityID entity, const std::string& functionName);

private:
    // 初始化脚本
    void InitializeScript(EntityID entity, ScriptComponent* script);

    // 更新脚本
    void UpdateScript(EntityID entity, ScriptComponent* script, float deltaTime);

    // 脚本执行环境
    void* m_scriptContext = nullptr;
};

// 变换系统 - 更新Transform组件的层次结构
class TransformSystem : public ISystem {
public:
    static constexpr SystemTypeID TYPE_ID = 6;

    SystemTypeID GetTypeID() const override { return TYPE_ID; }

    void Initialize() override;

    void Update(float deltaTime) override;

    void Shutdown() override;

    // 设置父实体
    void SetParent(EntityID entity, EntityID parent);

    // 获取子实体
    std::vector<EntityID> GetChildren(EntityID entity);

    // 获取世界变换矩阵
    DirectX::XMMATRIX GetWorldMatrix(EntityID entity);

private:
    // 层次结构
    struct HierarchyData {
        EntityID parent = INVALID_ENTITY;
        std::vector<EntityID> children;
        bool worldMatrixDirty = true;
        DirectX::XMMATRIX worldMatrix = DirectX::XMMatrixIdentity();
    };

    std::unordered_map<EntityID, HierarchyData> m_hierarchy;

    // 更新世界矩阵
    void UpdateWorldMatrix(EntityID entity);

    // 标记子节点为脏
    void MarkChildrenDirty(EntityID entity);
};

// 光照系统 - 更新光源组件
class LightSystem : public ISystem {
public:
    static constexpr SystemTypeID TYPE_ID = 7;

    SystemTypeID GetTypeID() const override { return TYPE_ID; }

    void Initialize() override;

    void Update(float deltaTime) override;

    void Shutdown() override;

    // 获取所有活动光源
    const std::vector<EntityID>& GetActiveLights() const { return m_activeLights; }

    // 设置环境光
    void SetAmbientLight(const DirectX::XMFLOAT3& ambient) { m_ambientLight = ambient; }
    const DirectX::XMFLOAT3& GetAmbientLight() const { return m_ambientLight; }

private:
    std::vector<EntityID> m_activeLights;
    DirectX::XMFLOAT3 m_ambientLight = DirectX::XMFLOAT3(0.1f, 0.1f, 0.1f);

    // 收集活动光源
    void CollectActiveLights();
};

// 相机系统 - 更新相机组件
class CameraSystem : public ISystem {
public:
    static constexpr SystemTypeID TYPE_ID = 8;

    SystemTypeID GetTypeID() const override { return TYPE_ID; }

    void Initialize() override;

    void Update(float deltaTime) override;

    void Shutdown() override;

    // 获取主相机
    EntityID GetMainCamera() const { return m_mainCamera; }

    // 获取视图矩阵
    DirectX::XMMATRIX GetViewMatrix(EntityID entity);

    // 获取投影矩阵
    DirectX::XMMATRIX GetProjectionMatrix(EntityID entity, float aspectRatio);

private:
    EntityID m_mainCamera = INVALID_ENTITY;

    // 查找主相机
    void FindMainCamera();

    // 更新相机矩阵
    void UpdateCameraMatrices(EntityID entity);
};

} // namespace ECS
} // namespace Core
} // namespace Engine