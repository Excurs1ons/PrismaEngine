#pragma once

#include "ECS.h"
#include <DirectXMath.h>
#include <string>

namespace Engine {
namespace Core {
namespace ECS {

// 组件类型ID定义
#define REGISTER_COMPONENT_TYPE(type) \
    namespace ComponentTypes { \
        static const ComponentTypeID type = 0x ## type; \
    }

// 基础组件类型
namespace ComponentTypes {
    static const ComponentTypeID Transform = 1;
    static const ComponentTypeID MeshRenderer = 2;
    static const ComponentTypeID Camera = 3;
    static const ComponentTypeID Light = 4;
    static const ComponentTypeID RigidBody = 5;
    static const ComponentTypeID Collider = 6;
    static const ComponentTypeID AudioSource = 7;
    static const ComponentTypeID Animation = 8;
    static const ComponentTypeID Script = 9;
}

// 变换组件 - 存储位置、旋转、缩放
class TransformComponent : public IComponent {
public:
    static constexpr ComponentTypeID TYPE_ID = ComponentTypes::Transform;

    TransformComponent() {
        position = DirectX::XMFLOAT3(0, 0, 0);
        rotation = DirectX::XMFLOAT4(0, 0, 0, 1); // Quaternion
        scale = DirectX::XMFLOAT3(1, 1, 1);
        UpdateMatrix();
    }

    ComponentTypeID GetTypeID() const override { return TYPE_ID; }

    // 获取世界矩阵
    const DirectX::XMMATRIX& GetWorldMatrix() const {
        return m_worldMatrix;
    }

    // 获取前向向量
    DirectX::XMVECTOR GetForward() const {
        auto quat = DirectX::XMLoadFloat4(&rotation);
        return DirectX::XMVector3Rotate(DirectX::XMVectorSet(0, 0, 1, 0), quat);
    }

    // 获取右向量
    DirectX::XMVECTOR GetRight() const {
        auto quat = DirectX::XMLoadFloat4(&rotation);
        return DirectX::XMVector3Rotate(DirectX::XMVectorSet(1, 0, 0, 0), quat);
    }

    // 获取上向量
    DirectX::XMVECTOR GetUp() const {
        auto quat = DirectX::XMLoadFloat4(&rotation);
        return DirectX::XMVector3Rotate(DirectX::XMVectorSet(0, 1, 0, 0), quat);
    }

    // 设置位置
    void SetPosition(const DirectX::XMFLOAT3& pos) {
        position = pos;
        UpdateMatrix();
    }

    // 设置旋转（欧拉角）
    void SetEulerAngles(const DirectX::XMFLOAT3& euler) {
        auto quat = DirectX::XMQuaternionRotationRollPitchYaw(
            DirectX::XMConvertToRadians(euler.x),
            DirectX::XMConvertToRadians(euler.y),
            DirectX::XMConvertToRadians(euler.z)
        );
        DirectX::XMStoreFloat4(&rotation, quat);
        UpdateMatrix();
    }

    // 设置缩放
    void SetScale(const DirectX::XMFLOAT3& s) {
        scale = s;
        UpdateMatrix();
    }

    // 变换矩阵属性
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT4 rotation; // Quaternion
    DirectX::XMFLOAT3 scale;

private:
    DirectX::XMMATRIX m_worldMatrix;

    void UpdateMatrix() {
        auto translation = DirectX::XMMatrixTranslation(position.x, position.y, position.z);
        auto quat = DirectX::XMLoadFloat4(&rotation);
        auto rotationMatrix = DirectX::XMMatrixRotationQuaternion(quat);
        auto scaleMatrix = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);

        m_worldMatrix = scaleMatrix * rotationMatrix * translation;
    }
};

// 网格渲染器组件
class MeshRendererComponent : public IComponent {
public:
    static constexpr ComponentTypeID TYPE_ID = ComponentTypes::MeshRenderer;

    ComponentTypeID GetTypeID() const override { return TYPE_ID; }

    // 资源路径
    std::string meshPath;
    std::string materialPath;

    // 渲染属性
    bool castShadows = true;
    bool receiveShadows = true;
    uint32_t renderLayer = 0;
};

// 相机组件
class CameraComponent : public IComponent {
public:
    static constexpr ComponentTypeID TYPE_ID = ComponentTypes::Camera;

    ComponentTypeID GetTypeID() const override { return TYPE_ID; }

    // 投影参数
    enum class ProjectionType {
        Perspective,
        Orthographic
    };

    ProjectionType projectionType = ProjectionType::Perspective;
    float fov = DirectX::XM_PIDIV4; // 45度
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    float orthoSize = 10.0f;

    // 获取投影矩阵
    DirectX::XMMATRIX GetProjectionMatrix(float aspectRatio) const {
        if (projectionType == ProjectionType::Perspective) {
            return DirectX::XMMatrixPerspectiveFovLH(fov, aspectRatio, nearPlane, farPlane);
        } else {
            float halfSize = orthoSize * 0.5f;
            return DirectX::XMMatrixOrthographicLH(
                halfSize * aspectRatio, halfSize, nearPlane, farPlane
            );
        }
    }

    // 主相机标记
    bool isMainCamera = false;
};

// 光源组件
class LightComponent : public IComponent {
public:
    static constexpr ComponentTypeID TYPE_ID = ComponentTypes::Light;

    ComponentTypeID GetTypeID() const override { return TYPE_ID; }

    enum class LightType {
        Directional,
        Point,
        Spot
    };

    LightType type = LightType::Point;

    // 颜色和强度
    DirectX::XMFLOAT3 color = DirectX::XMFLOAT3(1, 1, 1);
    float intensity = 1.0f;

    // 范围（点光源和聚光灯）
    float range = 10.0f;

    // 聚光灯参数
    float innerConeAngle = DirectX::XMConvertToRadians(30.0f);
    float outerConeAngle = DirectX::XMConvertToRadians(45.0f);

    // 阴影
    bool castShadows = false;
    uint32_t shadowMapSize = 1024;
};

// 刚体组件
class RigidBodyComponent : public IComponent {
public:
    static constexpr ComponentTypeID TYPE_ID = ComponentTypes::RigidBody;

    ComponentTypeID GetTypeID() const override { return TYPE_ID; }

    // 物理属性
    float mass = 1.0f;
    float friction = 0.5f;
    float restitution = 0.0f; // 弹性
    float linearDamping = 0.0f;
    float angularDamping = 0.0f;

    // 运动学模式（不受力影响）
    bool isKinematic = false;

    // 使用重力
    bool useGravity = true;

    // 冻结位置/旋转
    bool freezePositionX = false;
    bool freezePositionY = false;
    bool freezePositionZ = false;
    bool freezeRotationX = false;
    bool freezeRotationY = false;
    bool freezeRotationZ = false;
};

// 碰撞体组件（基类）
class ColliderComponent : public IComponent {
public:
    static constexpr ComponentTypeID TYPE_ID = ComponentTypes::Collider;

    ComponentTypeID GetTypeID() const override { return TYPE_ID; }

    enum class ColliderType {
        Box,
        Sphere,
        Capsule,
        Mesh
    };

    ColliderType type = ColliderType::Box;

    // 中心偏移
    DirectX::XMFLOAT3 center = DirectX::XMFLOAT3(0, 0, 0);

    // 尺寸
    DirectX::XMFLOAT3 size = DirectX::XMFLOAT3(1, 1, 1); // Box
    float radius = 0.5f; // Sphere
    float height = 2.0f; // Capsule

    // 触发器（不产生物理响应）
    bool isTrigger = false;

    // 物理材质
    float friction = 0.5f;
    float restitution = 0.0f;
};

// 音频源组件
class AudioSourceComponent : public IComponent {
public:
    static constexpr ComponentTypeID TYPE_ID = ComponentTypes::AudioSource;

    ComponentTypeID GetTypeID() const override { return TYPE_ID; }

    // 音频资源
    std::string audioClipPath;

    // 播放属性
    bool playOnAwake = false;
    bool loop = false;
    float volume = 1.0f;
    float pitch = 1.0f;
    float stereoPan = 0.0f;

    // 3D音频
    bool spatialBlend = false;
    float minDistance = 1.0f;
    float maxDistance = 500.0f;

    // 当前状态
    bool isPlaying = false;
    bool isPaused = false;
};

// 动画组件
class AnimationComponent : public IComponent {
public:
    static constexpr ComponentTypeID TYPE_ID = ComponentTypes::Animation;

    ComponentTypeID GetTypeID() const override { return TYPE_ID; }

    // 动画资源
    std::string animationPath;

    // 播放控制
    bool playOnAwake = false;
    bool loop = false;
    float playbackSpeed = 1.0f;

    // 当前状态
    float currentTime = 0.0f;
    bool isPlaying = false;
};

// 脚本组件
class ScriptComponent : public IComponent {
public:
    static constexpr ComponentTypeID TYPE_ID = ComponentTypes::Script;

    ComponentTypeID GetTypeID() const override { return TYPE_ID; }

    // 脚本文件路径
    std::vector<std::string> scriptPaths;

    // 脚本实例（运行时）
    std::vector<void*> scriptInstances;
};

} // namespace ECS
} // namespace Core
} // namespace Engine