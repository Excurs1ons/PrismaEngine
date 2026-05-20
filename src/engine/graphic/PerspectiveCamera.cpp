#include "PerspectiveCamera.h"

namespace Prisma {
namespace Graphic {

PerspectiveCamera::PerspectiveCamera()
    : m_fov(glm::radians(70.0f)), m_aspectRatio(16.0f / 9.0f),
      m_nearPlane(0.1f), m_farPlane(1000.0f) {
    RecalculateMatrices();
}

PerspectiveCamera::PerspectiveCamera(float fov, float aspectRatio, float nearPlane, float farPlane)
    : m_fov(fov), m_aspectRatio(aspectRatio),
      m_nearPlane(nearPlane), m_farPlane(farPlane) {
    RecalculateMatrices();
}

void PerspectiveCamera::SetFOV(float fov) {
    m_fov = fov;
    m_isProjectionDirty = true;
}

void PerspectiveCamera::SetNearFarPlanes(float nearPlane, float farPlane) {
    m_nearPlane = nearPlane;
    m_farPlane = farPlane;
    m_isProjectionDirty = true;
}

void PerspectiveCamera::SetAspectRatio(float aspectRatio) {
    m_aspectRatio = aspectRatio;
    m_isProjectionDirty = true;
}

void PerspectiveCamera::SetViewport(uint32_t width, uint32_t height) {
    m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    m_isProjectionDirty = true;
}

void PerspectiveCamera::SetClearColor(float r, float g, float b, float a) {
    m_clearColor = PrismaMath::vec4(r, g, b, a);
}

void PerspectiveCamera::SetPosition(const PrismaMath::vec3& pos) {
    m_position = pos;
    m_isViewDirty = true;
}

void PerspectiveCamera::SetRotation(const glm::quat& rotation) {
    m_rotation = rotation;
    m_isViewDirty = true;
}

void PerspectiveCamera::LookAt(const PrismaMath::vec3& target) {
    PrismaMath::vec3 direction = glm::normalize(target - m_position);
    PrismaMath::vec3 worldUp = PrismaMath::vec3(0.0f, 1.0f, 0.0f);

    // 防止 direction 与 worldUp 平行
    if (std::abs(glm::dot(direction, worldUp)) > 0.9999f) {
        worldUp = PrismaMath::vec3(0.0f, 0.0f, 1.0f);
    }

    PrismaMath::vec3 right = glm::normalize(glm::cross(worldUp, direction));
    PrismaMath::vec3 up = glm::cross(direction, right);

    PrismaMath::mat4 rotationMatrix = PrismaMath::mat4(1.0f);
    rotationMatrix[0][0] = right.x; rotationMatrix[0][1] = up.x; rotationMatrix[0][2] = direction.x;
    rotationMatrix[1][0] = right.y; rotationMatrix[1][1] = up.y; rotationMatrix[1][2] = direction.y;
    rotationMatrix[2][0] = right.z; rotationMatrix[2][1] = up.z; rotationMatrix[2][2] = direction.z;

    m_rotation = glm::quat_cast(rotationMatrix);
    m_isViewDirty = true;
}

void PerspectiveCamera::LookAt(float x, float y, float z) {
    LookAt(PrismaMath::vec3(x, y, z));
}

void PerspectiveCamera::MoveWorld(const PrismaMath::vec3& delta) {
    m_position += delta;
    m_isViewDirty = true;
}

void PerspectiveCamera::SetLookAt(const PrismaMath::vec3& position, const PrismaMath::vec3& target, const PrismaMath::vec3& up) {
    m_position = position;
    PrismaMath::vec3 direction = glm::normalize(target - position);
    PrismaMath::vec3 worldUp = up;

    if (std::abs(glm::dot(direction, worldUp)) > 0.9999f) {
        worldUp = PrismaMath::vec3(0.0f, 0.0f, 1.0f);
    }

    PrismaMath::vec3 right = glm::normalize(glm::cross(worldUp, direction));
    PrismaMath::vec3 camUp = glm::cross(direction, right);

    PrismaMath::mat4 rotationMatrix = PrismaMath::mat4(1.0f);
    rotationMatrix[0][0] = right.x; rotationMatrix[0][1] = camUp.x; rotationMatrix[0][2] = direction.x;
    rotationMatrix[1][0] = right.y; rotationMatrix[1][1] = camUp.y; rotationMatrix[1][2] = direction.y;
    rotationMatrix[2][0] = right.z; rotationMatrix[2][1] = camUp.z; rotationMatrix[2][2] = direction.z;

    m_rotation = glm::quat_cast(rotationMatrix);
    m_isViewDirty = true;
}

PrismaMath::mat4 PerspectiveCamera::GetViewMatrix() const {
    RecalculateMatrices();
    return m_viewMatrix;
}

PrismaMath::mat4 PerspectiveCamera::GetProjectionMatrix() const {
    if (m_isProjectionDirty) {
        m_projectionMatrix = glm::perspectiveRH(m_fov, m_aspectRatio, m_nearPlane, m_farPlane);
        m_isProjectionDirty = false;
    }
    return m_projectionMatrix;
}

PrismaMath::mat4 PerspectiveCamera::GetViewProjectionMatrix() const {
    return GetProjectionMatrix() * GetViewMatrix();
}

void PerspectiveCamera::RecalculateMatrices() const {
    if (!m_isViewDirty) return;

    PrismaMath::mat4 rotationMatrix = glm::mat4_cast(m_rotation);
    PrismaMath::mat4 translation = glm::translate(PrismaMath::mat4(1.0f), -m_position);
    m_viewMatrix = glm::transpose(rotationMatrix) * translation;

    RecalculateVectors();
    m_isViewDirty = false;
}

void PerspectiveCamera::RecalculateVectors() const {
    PrismaMath::mat4 rotationMatrix = glm::mat4_cast(m_rotation);
    m_forward = glm::normalize(PrismaMath::vec3(rotationMatrix[2][0], rotationMatrix[2][1], rotationMatrix[2][2]));
    m_up      = glm::normalize(PrismaMath::vec3(rotationMatrix[1][0], rotationMatrix[1][1], rotationMatrix[1][2]));
    m_right   = glm::normalize(PrismaMath::vec3(rotationMatrix[0][0], rotationMatrix[0][1], rotationMatrix[0][2]));
}

} // namespace Graphic
} // namespace Prisma
