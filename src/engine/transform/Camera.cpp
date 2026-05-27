#include "Camera.h"
#include "Transform.h"
#include "Logger.h"
#include "core/ComponentRegistry.h"
#include <glaze/glaze.hpp>

// Glaze 元数据

template <>
struct glz::meta<Prisma::Graphic::ProjectionMode> {
    using enum Prisma::Graphic::ProjectionMode;
    static constexpr auto value = glz::enumerate(
        "perspective",  Perspective,
        "orthographic", Orthographic
    );
};

template <>
struct glz::meta<Prisma::Graphic::Camera::Data> {
    static constexpr auto value = glz::object(
        "projectionMode", &Prisma::Graphic::Camera::Data::projectionMode,
        "fov",            &Prisma::Graphic::Camera::Data::fovDeg,
        "orthoSize",      &Prisma::Graphic::Camera::Data::orthoSize,
        "near",           &Prisma::Graphic::Camera::Data::nearPlane,
        "far",            &Prisma::Graphic::Camera::Data::farPlane,
        "clearColor",     &Prisma::Graphic::Camera::Data::clearColor
    );
};

// ComponentRegistry 注册
namespace {
    bool registered = []() {
        auto& reg = Prisma::ComponentRegistry::Get();
        reg.Register<Prisma::Graphic::Camera>("Camera");
        reg.RegisterSerializable("Camera",
            [](const Prisma::Component& comp) -> std::string {
                const auto& typed = static_cast<const Prisma::Graphic::Camera&>(comp);
                auto data = typed.GetData();
                std::string json;
                auto ec = glz::write_json(data, json);
                if (ec) json.clear();
                return json;
            },
            [](Prisma::Component& comp, const std::string& json) {
                auto& typed = static_cast<Prisma::Graphic::Camera&>(comp);
                Prisma::Graphic::Camera::Data data;
                auto ec = glz::read_json(data, json);
                if (!ec) typed.SetData(data);
            }
        );
        return true;
    }();
}

namespace Prisma {
namespace Graphic {

Camera::Camera()
    : m_fov(Prisma::PI / 4.0f), m_orthoSize(5.0f), m_aspectRatio(16.0f / 9.0f),
      m_nearPlane(0.1f), m_farPlane(1000.0f),
      m_isViewDirty(true), m_isProjectionDirty(true) {
    m_forward = PrismaMath::vec3(0.0f, 0.0f, 1.0f);
    m_up      = PrismaMath::vec3(0.0f, 1.0f, 0.0f);
    m_right   = PrismaMath::vec3(1.0f, 0.0f, 0.0f);
    m_viewMatrix       = PrismaMath::mat4(1.0f);
    m_projectionMatrix = PrismaMath::mat4(1.0f);
}

Camera::~Camera() {}

void Camera::Initialize() {
    LOG_DEBUG("Camera3D", "Node '{0}' 的 Camera3D 组件已初始化", GetNodeName());
    MarkViewDirty();
}

void Camera::Update(Timestep ts) {
    if (ts.GetSeconds() > 0.0f && m_farPlane < m_nearPlane) {
        std::swap(m_farPlane, m_nearPlane);
        m_isProjectionDirty = true;
    }
    UpdateViewMatrix();
}

// 投影模式

void Camera::SetProjectionMode(ProjectionMode mode) {
    m_projectionMode = mode;
    m_isProjectionDirty = true;
}

void Camera::SetPerspectiveProjection(float fov, float aspectRatio, float nearPlane, float farPlane) {
    m_projectionMode    = ProjectionMode::Perspective;
    m_fov               = fov;
    m_aspectRatio       = aspectRatio;
    m_nearPlane         = nearPlane;
    m_farPlane          = farPlane;
    m_isProjectionDirty = true;
}

void Camera::SetOrthographicProjection(float size, float aspectRatio, float nearPlane, float farPlane) {
    m_projectionMode    = ProjectionMode::Orthographic;
    m_orthoSize         = size;
    m_aspectRatio       = aspectRatio;
    m_nearPlane         = nearPlane;
    m_farPlane          = farPlane;
    m_isProjectionDirty = true;
}

void Camera::SetOrthoSize(float size) {
    m_orthoSize = size;
    m_isProjectionDirty = true;
}

// 数据序列化

Camera::Data Camera::GetData() const {
    Data d;
    d.projectionMode = m_projectionMode;
    d.fovDeg         = glm::degrees(m_fov);
    d.orthoSize      = m_orthoSize;
    d.nearPlane      = m_nearPlane;
    d.farPlane       = m_farPlane;
    d.clearColor     = {m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a};
    return d;
}

void Camera::SetData(const Data& d) {
    m_projectionMode    = d.projectionMode;
    m_fov               = glm::radians(d.fovDeg);
    m_orthoSize         = d.orthoSize;
    m_nearPlane         = d.nearPlane;
    m_farPlane          = d.farPlane;
    m_clearColor        = PrismaMath::vec4(d.clearColor[0], d.clearColor[1], d.clearColor[2], d.clearColor[3]);
    m_isProjectionDirty = true;
}

// ICamera 接口

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
        if (m_projectionMode == ProjectionMode::Orthographic) {
            float halfH = m_orthoSize * 0.5f;
            float halfW = halfH * m_aspectRatio;
            m_projectionMatrix = glm::orthoLH_ZO(-halfW, halfW, -halfH, halfH, m_nearPlane, m_farPlane);
        } else {
            // 使用 LH 版本，使得 +Z 轴指向屏幕内
            // [修复] m_fov 内部始终存储为弧度（由 SetData 或 SetPerspectiveProjection 保证）
            m_projectionMatrix = glm::perspectiveLH_ZO(m_fov, m_aspectRatio, m_nearPlane, m_farPlane);
            
            // [关键修复] Vulkan Y轴翻转
            // Vulkan 的 NDC Y轴是向下的，通过翻转投影矩阵的 [1][1] 来适配 Y-up 坐标系
            m_projectionMatrix[1][1] *= -1.0f;
        }
        m_isProjectionDirty = false;
    }
    return m_projectionMatrix;
}

PrismaMath::mat4 Camera::GetViewProjectionMatrix() const {
    return GetProjectionMatrix() * GetViewMatrix();
}

PrismaMath::vec3 Camera::GetPosition() const {
    if (auto transform = GetTransform()) {
        return transform->GetPosition();
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
    m_fov               = fov;
    m_isProjectionDirty = true;
}

void Camera::SetNearFarPlanes(float nearPlane, float farPlane) {
    m_nearPlane         = nearPlane;
    m_farPlane          = farPlane;
    m_isProjectionDirty = true;
}

void Camera::SetAspectRatio(float aspectRatio) {
    m_aspectRatio       = aspectRatio;
    m_isProjectionDirty = true;
}

void Camera::MoveWorld(float x, float y, float z) {
    if (auto transform = GetTransform()) {
        Prisma::Vector3 pos = transform->GetPosition();
        pos.x += x;
        pos.y += y;
        pos.z += z;
        transform->SetPosition(pos);
        MarkViewDirty();
    }
}

void Camera::MoveWorld(const PrismaMath::vec3& direction) {
    if (auto transform = GetTransform()) {
        transform->SetPosition(transform->GetPosition() + direction);
        MarkViewDirty();
    }
}

void Camera::MoveLocal(float forward, float right, float up) {
    UpdateVectors();
    PrismaMath::vec3 movement = PrismaMath::vec3(0.0f, 0.0f, 0.0f);
    if (forward != 0.0f)
        movement = movement + m_forward * forward;
    if (right != 0.0f)
        movement = movement + m_right * right;
    if (up != 0.0f)
        movement = movement + m_up * up;
    MoveWorld(movement);
}

void Camera::Rotate(float pitch, float yaw, float roll) {
    if (auto transform = GetTransform()) {
        glm::quat deltaRotation = glm::quat(glm::vec3(glm::radians(pitch), glm::radians(yaw), glm::radians(roll)));
        glm::quat newRotation  = deltaRotation * transform->GetRotation();
        transform->SetRotation(newRotation);
        MarkViewDirty();
    }
}

void Camera::LookAt(const PrismaMath::vec3& target) {
    if (auto transform = GetTransform()) {
        PrismaMath::vec3 position  = GetPosition();
        // LH LookAt: forward points from camera to target
        m_viewMatrix = glm::lookAtLH(position, target, PrismaMath::vec3(0.0f, 1.0f, 0.0f));
        transform->SetRotation(glm::quat_cast(glm::inverse(m_viewMatrix)));
        MarkViewDirty();
    }
}

void Camera::LookAt(float x, float y, float z) {
    LookAt(PrismaMath::vec3(x, y, z));
}

void Camera::UpdateViewMatrix() const {
    if (!m_isViewDirty) return;

    if (auto transform = GetTransform()) {
        PrismaMath::vec3 position   = transform->GetPosition();
        glm::quat rotation          = transform->GetRotation();
        PrismaMath::mat4 rotationMatrix = glm::mat4_cast(rotation);

        // LH: Z轴是 Forward
        m_forward = glm::normalize(
            PrismaMath::vec3(rotationMatrix[2][0], rotationMatrix[2][1], rotationMatrix[2][2]));
        m_up = glm::normalize(
            PrismaMath::vec3(rotationMatrix[1][0], rotationMatrix[1][1], rotationMatrix[1][2]));
        m_right = glm::normalize(
            PrismaMath::vec3(rotationMatrix[0][0], rotationMatrix[0][1], rotationMatrix[0][2]));

        // LH View Matrix: R^T * T^-1
        // 但 GLM 的 lookAtLH 更可靠
        m_viewMatrix = glm::lookAtLH(position, position + m_forward, m_up);

        m_isViewDirty = false;
    }
}

void Camera::UpdateVectors() const {
    UpdateViewMatrix();
}

void Camera::MarkViewDirty() const {
    m_isViewDirty = true;
}

}  // namespace Graphic
}  // namespace Prisma
