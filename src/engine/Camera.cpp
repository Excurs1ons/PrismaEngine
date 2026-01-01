#include "Camera.h"
#include "GameObject.h"
#include "Logger.h"
#include "math/Math.h"

namespace Engine {
namespace Graphic {

Camera::Camera()
    : m_fov(Prisma::Math::PI / 4.0f)
    , m_aspectRatio(16.0f / 9.0f)
    , m_nearPlane(0.1f)
    , m_farPlane(1000.0f)
    , m_clearColor(0.0f, 0.0f, 0.0f, 1.0f)
    , m_isViewDirty(true)
    , m_isProjectionDirty(true)
{
    // 初始化缓存向量
    m_forward = PrismaMath::vec3(0.0f, 0.0f, 1.0f);
    m_up = PrismaMath::vec3(0.0f, 1.0f, 0.0f);
    m_right = PrismaMath::vec3(1.0f, 0.0f, 0.0f);

    // 初始化矩阵为单位矩阵
    m_viewMatrix = PrismaMath::mat4(1.0f);
    m_projectionMatrix = PrismaMath::mat4(1.0f);
}

Camera::~Camera() {
}

void Camera::Initialize() {
    LOG_INFO("Camera3D", "Camera3D component initialized for GameObject '{0}'", m_owner->name);

    // 初始化Transform的旋转（相机默认看向-Z方向）
    if (auto transform = m_owner->transform()) {
        // 设置初始旋转为 Identity
        transform->rotation = Prisma::Math::QuaternionIdentity();
        MarkViewDirty();
    }
}

void Camera::Update(float deltaTime) {
    // 更新视图矩阵（如果需要）
    UpdateViewMatrix();
}

void Camera::SetPerspectiveProjection(float fov, float aspectRatio, float nearPlane, float farPlane) {
    m_fov = fov;
    m_aspectRatio = aspectRatio;
    m_nearPlane = nearPlane;
    m_farPlane = farPlane;
    m_isProjectionDirty = true;
}

PrismaMath::vec4 Camera::GetClearColor() const {
    return m_clearColor;
}

void Camera::SetClearColor(float r, float g, float b, float a) {
    m_clearColor = PrismaMath::vec4(r, g, b, a);
}

PrismaMath::mat4 Camera::GetViewMatrix() const {
    UpdateViewMatrix();
    return m_viewMatrix;
}

PrismaMath::mat4 Camera::GetProjectionMatrix() const {
    if (m_isProjectionDirty) {
        m_projectionMatrix = Prisma::Math::Perspective(m_fov, m_aspectRatio, m_nearPlane, m_farPlane);
        m_isProjectionDirty = false;
    }
    return m_projectionMatrix;
}

PrismaMath::mat4 Camera::GetViewProjectionMatrix() const {
    return Prisma::Math::Multiply(GetViewMatrix(), GetProjectionMatrix());
}

PrismaMath::vec3 Camera::GetPosition() const {
    if (auto transform = m_owner->transform()) {
        return PrismaMath::vec3(transform->position.x, transform->position.y, transform->position.z);
    }
    return PrismaMath::vec3(0.0f, 0.0f, 0.0f);
}

PrismaMath::vec3 Camera::GetForward() const {
    UpdateVectors();
    return m_forward;
}

PrismaMath::vec3 Camera::GetUp() const {
    UpdateVectors();
    return m_up;
}

PrismaMath::vec3 Camera::GetRight() const {
    UpdateVectors();
    return m_right;
}

void Camera::SetFOV(float fov) {
    m_fov = fov;
    m_isProjectionDirty = true;
}

void Camera::SetNearFarPlanes(float nearPlane, float farPlane) {
    m_nearPlane = nearPlane;
    m_farPlane = farPlane;
    m_isProjectionDirty = true;
}

void Camera::SetAspectRatio(float aspectRatio) {
    m_aspectRatio = aspectRatio;
    m_isProjectionDirty = true;
}

void Camera::MoveWorld(float x, float y, float z) {
    if (auto transform = m_owner->transform()) {
        transform->position.x += x;
        transform->position.y += y;
        transform->position.z += z;
        MarkViewDirty();
    }
}

void Camera::MoveWorld(const PrismaMath::vec3& direction) {
    if (auto transform = m_owner->transform()) {
        transform->position.x += direction.x;
        transform->position.y += direction.y;
        transform->position.z += direction.z;
        MarkViewDirty();
    }
}

void Camera::MoveLocal(float forward, float right, float up) {
    UpdateVectors();

    // 计算移动向量
    PrismaMath::vec3 movement = PrismaMath::vec3(0.0f, 0.0f, 0.0f);
    if (forward != 0.0f) movement = movement + m_forward * forward;
    if (right != 0.0f) movement = movement + m_right * right;
    if (up != 0.0f) movement = movement + m_up * up;

    MoveWorld(movement);
}

void Camera::Rotate(float pitch, float yaw, float roll) {
    if (auto transform = m_owner->transform()) {
        // 创建旋转增量（弧度）
        glm::quat deltaRotation = Prisma::Math::FromEulerAngles(glm::vec3(
            Prisma::Math::Radians(pitch),
            Prisma::Math::Radians(yaw),
            Prisma::Math::Radians(roll)
        ));

        // 应用旋转到当前旋转
        glm::quat currentRotation = transform->rotation;
        glm::quat newRotation = deltaRotation * currentRotation;
        transform->rotation = newRotation;

        MarkViewDirty();
    }
}

void Camera::LookAt(const PrismaMath::vec3& target) {
    if (auto transform = m_owner->transform()) {
        PrismaMath::vec3 position = GetPosition();
        PrismaMath::vec3 direction = Prisma::Math::Normalize(target - position);

        // 创建前向向量（相机看向-Z方向）
        PrismaMath::vec3 forward = -direction;

        // 计算上向量
        PrismaMath::vec3 worldUp = PrismaMath::vec3(0.0f, 1.0f, 0.0f);
        PrismaMath::vec3 right = Prisma::Math::Normalize(Prisma::Math::Cross(worldUp, forward));
        PrismaMath::vec3 up = Prisma::Math::Cross(forward, right);

        // 创建旋转矩阵
        PrismaMath::mat4 rotationMatrix = PrismaMath::mat4(1.0f);
        rotationMatrix[0][0] = right.x;
        rotationMatrix[0][1] = up.x;
        rotationMatrix[0][2] = forward.x;
        rotationMatrix[1][0] = right.y;
        rotationMatrix[1][1] = up.y;
        rotationMatrix[1][2] = forward.y;
        rotationMatrix[2][0] = right.z;
        rotationMatrix[2][1] = up.z;
        rotationMatrix[2][2] = forward.z;

        // 转换为四元数
        glm::quat rotationQuat = glm::quat_cast(rotationMatrix);
        transform->rotation = rotationQuat;
        MarkViewDirty();
    }
}

void Camera::LookAt(float x, float y, float z) {
    LookAt(PrismaMath::vec3(x, y, z));
}

void Camera::UpdateViewMatrix() const {
    if (!m_isViewDirty) {
        return;
    }

    if (auto transform = m_owner->transform()) {
        // 获取位置和旋转
        PrismaMath::vec3 position = PrismaMath::vec3(transform->position.x, transform->position.y, transform->position.z);
        glm::quat rotation = transform->rotation;

        // 创建旋转矩阵
        PrismaMath::mat4 rotationMatrix = Prisma::Math::QuaternionToMatrix(rotation);

        // 相机默认前向是-Z，所以需要额外旋转
        PrismaMath::mat4 cameraFix = Prisma::Math::RotationY(Prisma::Math::PI);
        rotationMatrix = Prisma::Math::Multiply(cameraFix, rotationMatrix);

        // 计算世界坐标系的各轴
        m_forward = Prisma::Math::Normalize(PrismaMath::vec3(rotationMatrix[2][0], rotationMatrix[2][1], rotationMatrix[2][2]));  // Z轴（前向）
        m_up = Prisma::Math::Normalize(PrismaMath::vec3(rotationMatrix[1][0], rotationMatrix[1][1], rotationMatrix[1][2]));        // Y轴（上向）
        m_right = Prisma::Math::Normalize(PrismaMath::vec3(rotationMatrix[0][0], rotationMatrix[0][1], rotationMatrix[0][2]));     // X轴（右向）

        // 计算视图矩阵（相机变换的逆矩阵）
        // 视图矩阵 = 旋转矩阵的转置 * 平移矩阵的逆
        PrismaMath::mat4 translation = Prisma::Math::Translation(-position);
        m_viewMatrix = Prisma::Math::Transpose(rotationMatrix);
        m_viewMatrix = Prisma::Math::Multiply(m_viewMatrix, translation);

        m_isViewDirty = false;
    }
}

void Camera::UpdateVectors() const {
    UpdateViewMatrix();
}

void Camera::MarkViewDirty() const {
    m_isViewDirty = true;
}

} // namespace Graphic
} // namespace Engine