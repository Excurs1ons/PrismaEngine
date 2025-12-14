#include "Camera3D.h"
#include "GameObject.h"
#include "Logger.h"

using namespace DirectX;

namespace Engine {
namespace Graphic {

Camera3D::Camera3D()
    : m_fov(XM_PIDIV4)
    , m_aspectRatio(16.0f / 9.0f)
    , m_nearPlane(0.1f)
    , m_farPlane(1000.0f)
    , m_clearColor(0.0f, 0.0f, 0.0f, 1.0f)
    , m_isViewDirty(true)
    , m_isProjectionDirty(true)
{
    // 初始化缓存向量
    m_forward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    m_up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    m_right = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

    // 初始化矩阵为单位矩阵
    m_viewMatrix = XMMatrixIdentity();
    m_projectionMatrix = XMMatrixIdentity();
}

Camera3D::~Camera3D() {
}

void Camera3D::Initialize() {
    LOG_INFO("Camera3D", "Camera3D component initialized for GameObject '{0}'", m_owner->name);

    // 初始化Transform的旋转（相机默认看向-Z方向）
    if (auto transform = m_owner->transform()) {
        // 设置初始旋转为 Identity
        transform->rotation = Quaternion::Identity;
        MarkViewDirty();
    }
}

void Camera3D::Update(float deltaTime) {
    // 更新视图矩阵（如果需要）
    UpdateViewMatrix();
}

void Camera3D::SetPerspectiveProjection(float fov, float aspectRatio, float nearPlane, float farPlane) {
    m_fov = fov;
    m_aspectRatio = aspectRatio;
    m_nearPlane = nearPlane;
    m_farPlane = farPlane;
    m_isProjectionDirty = true;
}

DirectX::XMVECTOR Camera3D::GetClearColor() const {
    return XMLoadFloat4(&m_clearColor);
}

void Camera3D::SetClearColor(float r, float g, float b, float a) {
    m_clearColor = XMFLOAT4(r, g, b, a);
}

XMMATRIX Camera3D::GetViewMatrix() const {
    UpdateViewMatrix();
    return m_viewMatrix;
}

XMMATRIX Camera3D::GetProjectionMatrix() const {
    if (m_isProjectionDirty) {
        m_projectionMatrix = XMMatrixPerspectiveFovLH(m_fov, m_aspectRatio, m_nearPlane, m_farPlane);
        m_isProjectionDirty = false;
    }
    return m_projectionMatrix;
}

XMMATRIX Camera3D::GetViewProjectionMatrix() const {
    return XMMatrixMultiply(GetViewMatrix(), GetProjectionMatrix());
}

XMVECTOR Camera3D::GetPosition() const {
    if (auto transform = m_owner->transform()) {
        return XMLoadFloat3(&transform->position);
    }
    return XMVectorZero();
}

XMVECTOR Camera3D::GetForward() const {
    UpdateVectors();
    return m_forward;
}

XMVECTOR Camera3D::GetUp() const {
    UpdateVectors();
    return m_up;
}

XMVECTOR Camera3D::GetRight() const {
    UpdateVectors();
    return m_right;
}

void Camera3D::SetFOV(float fov) {
    m_fov = fov;
    m_isProjectionDirty = true;
}

void Camera3D::SetNearFarPlanes(float nearPlane, float farPlane) {
    m_nearPlane = nearPlane;
    m_farPlane = farPlane;
    m_isProjectionDirty = true;
}

void Camera3D::SetAspectRatio(float aspectRatio) {
    m_aspectRatio = aspectRatio;
    m_isProjectionDirty = true;
}

void Camera3D::MoveWorld(float x, float y, float z) {
    if (auto transform = m_owner->transform()) {
        transform->position.x += x;
        transform->position.y += y;
        transform->position.z += z;
        MarkViewDirty();
    }
}

void Camera3D::MoveWorld(FXMVECTOR direction) {
    if (auto transform = m_owner->transform()) {
        XMFLOAT3 currentPos = transform->position;
        XMVECTOR currentPosVec = XMLoadFloat3(&currentPos);
        XMVECTOR newPos = XMVectorAdd(currentPosVec, direction);
        XMStoreFloat3(&transform->position, newPos);
        MarkViewDirty();
    }
}

void Camera3D::MoveLocal(float forward, float right, float up) {
    UpdateVectors();

    // 计算移动向量
    XMVECTOR movement = XMVectorZero();
    if (forward != 0.0f) movement = XMVectorAdd(movement, XMVectorScale(m_forward, forward));
    if (right != 0.0f) movement = XMVectorAdd(movement, XMVectorScale(m_right, right));
    if (up != 0.0f) movement = XMVectorAdd(movement, XMVectorScale(m_up, up));

    MoveWorld(movement);
}

void Camera3D::Rotate(float pitch, float yaw, float roll) {
    if (auto transform = m_owner->transform()) {
        // 创建旋转增量（弧度）
        XMVECTOR deltaRotation = XMQuaternionRotationRollPitchYaw(
            XMConvertToRadians(pitch),
            XMConvertToRadians(yaw),
            XMConvertToRadians(roll)
        );

        // 应用旋转到当前旋转
        XMVECTOR currentRotation = transform->rotation.ToXMVector();
        XMVECTOR newRotation = XMQuaternionMultiply(deltaRotation, currentRotation);
        transform->rotation.FromXMVector(newRotation);

        MarkViewDirty();
    }
}

void Camera3D::LookAt(FXMVECTOR target) {
    if (auto transform = m_owner->transform()) {
        XMVECTOR position = GetPosition();
        XMVECTOR direction = XMVectorSubtract(target, position);
        direction = XMVector3Normalize(direction);

        // 创建前向向量（相机看向-Z方向）
        XMVECTOR forward = XMVectorNegate(direction);

        // 计算上向量
        XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        XMVECTOR right = XMVector3Normalize(XMVector3Cross(worldUp, forward));
        XMVECTOR up = XMVector3Cross(forward, right);

        // 创建旋转矩阵
        XMMATRIX rotationMatrix = XMMATRIX(
            XMVectorGetX(right), XMVectorGetX(up), XMVectorGetX(forward), 0.0f,
            XMVectorGetY(right), XMVectorGetY(up), XMVectorGetY(forward), 0.0f,
            XMVectorGetZ(right), XMVectorGetZ(up), XMVectorGetZ(forward), 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        );

        // 转换为四元数
        XMVECTOR rotationQuat = XMQuaternionRotationMatrix(rotationMatrix);
        transform->rotation.FromXMVector(rotationQuat);
        MarkViewDirty();
    }
}

void Camera3D::LookAt(float x, float y, float z) {
    LookAt(XMVectorSet(x, y, z, 0.0f));
}

void Camera3D::UpdateViewMatrix() const {
    if (!m_isViewDirty) {
        return;
    }

    if (auto transform = m_owner->transform()) {
        // 获取位置和旋转
        XMVECTOR position = XMLoadFloat3(&transform->position);
        XMVECTOR rotation = transform->rotation.ToXMVector();

        // 创建旋转矩阵
        XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(rotation);

        // 相机默认前向是-Z，所以需要额外旋转
        XMMATRIX cameraFix = XMMatrixRotationY(XM_PI);
        rotationMatrix = XMMatrixMultiply(cameraFix, rotationMatrix);

        // 计算世界坐标系的各轴
        m_forward = XMVector3Normalize(rotationMatrix.r[2]);  // Z轴（前向）
        m_up = XMVector3Normalize(rotationMatrix.r[1]);       // Y轴（上向）
        m_right = XMVector3Normalize(rotationMatrix.r[0]);    // X轴（右向）

        // 计算视图矩阵（相机变换的逆矩阵）
        // 视图矩阵 = 旋转矩阵的转置 * 平移矩阵的逆
        XMMATRIX translation = XMMatrixTranslationFromVector(-position);
        m_viewMatrix = XMMatrixTranspose(rotationMatrix);
        m_viewMatrix = XMMatrixMultiply(m_viewMatrix, translation);

        m_isViewDirty = false;
    }
}

void Camera3D::UpdateVectors() const {
    UpdateViewMatrix();
}

void Camera3D::MarkViewDirty() const {
    m_isViewDirty = true;
}

} // namespace Graphic
} // namespace Engine