#include "Camera3D.h"
#include "GameObject.h"
#include <DirectXMath.h>

using namespace DirectX;

Camera3D::Camera3D() :
    m_position(XMVectorSet(0.0f, 0.0f, -5.0f, 1.0f)),
    m_pitch(0.0f),
    m_yaw(0.0f),
    m_roll(0.0f),
    m_fov(XM_PIDIV4),  // 45度
    m_aspectRatio(16.0f / 9.0f),
    m_nearPlane(0.1f),
    m_farPlane(1000.0f),
    m_isViewDirty(true),
    m_isProjectionDirty(true)
{
    UpdateVectors();
}

Camera3D::~Camera3D() = default;

void Camera3D::Initialize() {
    m_clearColor = XMVectorSet(0.1f, 0.2f, 0.3f, 1.0f);  // 深蓝色背景
    UpdateVectors();
}

void Camera3D::Update(float deltaTime) {
    // 更新视图矩阵（如果需要）
    if (m_isViewDirty) {
        UpdateViewMatrix();
    }

    // 更新投影矩阵（如果需要）
    if (m_isProjectionDirty) {
        m_projectionMatrix = XMMatrixPerspectiveFovLH(m_fov, m_aspectRatio, m_nearPlane, m_farPlane);
        m_isProjectionDirty = false;
    }
}

void Camera3D::SetPosition(float x, float y, float z) {
    m_position = XMVectorSet(x, y, z, 1.0f);
    m_isViewDirty = true;
}

void Camera3D::SetPosition(FXMVECTOR position) {
    m_position = position;
    m_isViewDirty = true;
}

XMVECTOR Camera3D::GetPosition() const {
    return m_position;
}

void Camera3D::SetRotation(float pitch, float yaw, float roll) {
    m_pitch = pitch;
    m_yaw = yaw;
    m_roll = roll;
    m_isViewDirty = true;
    UpdateVectors();
}

void Camera3D::SetPitch(float pitch) {
    m_pitch = pitch;
    m_isViewDirty = true;
    UpdateVectors();
}

void Camera3D::SetYaw(float yaw) {
    m_yaw = yaw;
    m_isViewDirty = true;
    UpdateVectors();
}

void Camera3D::SetRoll(float roll) {
    m_roll = roll;
    m_isViewDirty = true;
    UpdateVectors();
}

float Camera3D::GetPitch() const {
    return m_pitch;
}

float Camera3D::GetYaw() const {
    return m_yaw;
}

float Camera3D::GetRoll() const {
    return m_roll;
}

void Camera3D::SetPerspectiveProjection(float fov, float aspectRatio, float nearPlane, float farPlane) {
    m_fov = fov;
    m_aspectRatio = aspectRatio;
    m_nearPlane = nearPlane;
    m_farPlane = farPlane;
    m_isProjectionDirty = true;
}

XMMATRIX Camera3D::GetViewMatrix() const {
    if (m_isViewDirty) {
        UpdateViewMatrix();
    }
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
    return GetViewMatrix() * GetProjectionMatrix();
}

void Camera3D::UpdateProjectionMatrix(float windowWidth, float windowHeight) {
    m_aspectRatio = windowWidth / windowHeight;
    m_isProjectionDirty = true;
}

XMVECTOR Camera3D::GetForward() const {
    return m_forward;
}

XMVECTOR Camera3D::GetUp() const {
    return m_up;
}

XMVECTOR Camera3D::GetRight() const {
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
    m_position = XMVectorAdd(m_position, XMVectorSet(x, y, z, 0.0f));
    m_isViewDirty = true;
}

void Camera3D::MoveWorld(FXMVECTOR direction) {
    m_position = XMVectorAdd(m_position, direction);
    m_isViewDirty = true;
}

void Camera3D::MoveLocal(float forward, float right, float up) {
    XMVECTOR moveVector = XMVectorZero();

    if (forward != 0.0f) {
        moveVector = XMVectorAdd(moveVector, XMVectorScale(m_forward, forward));
    }
    if (right != 0.0f) {
        moveVector = XMVectorAdd(moveVector, XMVectorScale(m_right, right));
    }
    if (up != 0.0f) {
        moveVector = XMVectorAdd(moveVector, XMVectorScale(m_up, up));
    }

    m_position = XMVectorAdd(m_position, moveVector);
    m_isViewDirty = true;
}

void Camera3D::Rotate(float pitch, float yaw, float roll) {
    m_pitch += pitch;
    m_yaw += yaw;
    m_roll += roll;
    m_isViewDirty = true;
    UpdateVectors();
}

void Camera3D::LookAt(FXMVECTOR target) {
    XMVECTOR lookDirection = XMVectorSubtract(target, m_position);
    lookDirection = XMVector3Normalize(lookDirection);

    // 计算偏航角（Y轴旋转）
    m_yaw = atan2f(XMVectorGetX(lookDirection), XMVectorGetZ(lookDirection));

    // 计算俯仰角（X轴旋转）
    float horizontalDistance = sqrtf(XMVectorGetX(lookDirection) * XMVectorGetX(lookDirection) +
                                   XMVectorGetZ(lookDirection) * XMVectorGetZ(lookDirection));
    m_pitch = -atan2f(XMVectorGetY(lookDirection), horizontalDistance);

    m_isViewDirty = true;
    UpdateVectors();
}

void Camera3D::LookAt(float x, float y, float z) {
    LookAt(XMVectorSet(x, y, z, 1.0f));
}

void Camera3D::UpdateViewMatrix() const {
    // 使用旋转矩阵创建视图矩阵
    XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, m_roll);

    // 创建平移矩阵
    XMMATRIX translationMatrix = XMMatrixTranslationFromVector(m_position);

    // 视图矩阵 = 旋转矩阵的转置 * 平移矩阵的转置的逆
    m_viewMatrix = XMMatrixTranspose(rotationMatrix) * XMMatrixInverse(nullptr, translationMatrix);

    m_isViewDirty = false;
}

void Camera3D::UpdateVectors() const {
    // 使用欧拉角计算旋转
    XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, m_roll);

    // 前向量（相机看的方向）
    m_forward = XMVector3TransformCoord(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotationMatrix);
    m_forward = XMVector3Normalize(m_forward);

    // 上向量
    m_up = XMVector3TransformCoord(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), rotationMatrix);
    m_up = XMVector3Normalize(m_up);

    // 右向量
    m_right = XMVector3Cross(m_forward, m_up);
    m_right = XMVector3Normalize(m_right);
}