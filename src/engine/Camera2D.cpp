#include "Camera2D.h"
#include "math/Math.h"

Camera2D::Camera2D()
    : m_position(PrismaMath::vec4(0.0f, 0.0f, 0.0f, 1.0f))
    , m_rotation(0.0f)
    , m_viewMatrix(PrismaMath::mat4(1.0f))
    , m_projectionMatrix(PrismaMath::mat4(1.0f))
    , m_width(100.0f)
    , m_height(100.0f)
    , m_left(-50.0f)
    , m_right(50.0f)
    , m_bottom(-50.0f)
    , m_top(50.0f)
    , m_nearPlane(0.1f)
    , m_farPlane(1000.0f)
    , m_isOrthoDirty(true)
    , m_isViewDirty(true)
{
    m_clearColor = PrismaMath::vec4(0.0f, 1.0f, 1.0f, 1.0f);
}

Camera2D::~Camera2D()
{
}

void Camera2D::Initialize()
{
    // 初始化摄像机，可以在这里设置默认参数
}

void Camera2D::Update(float deltaTime)
{
    // 更新摄像机逻辑，可以在这里处理动画等
}

void Camera2D::SetPosition(float x, float y, float z)
{
    m_position = PrismaMath::vec4(x, y, z, 0.0f);
    m_isViewDirty = true;
}

void Camera2D::SetPosition(const PrismaMath::vec4& position)
{
    m_position = position;
    m_isViewDirty = true;
}

PrismaMath::vec4 Camera2D::GetPosition() const
{
    return m_position;
}

void Camera2D::SetRotation(float rotation)
{
    m_rotation = rotation;
    m_isViewDirty = true;
}

float Camera2D::GetRotation() const
{
    return m_rotation;
}

void Camera2D::SetOrthographicProjection(float width, float height, float nearPlane, float farPlane)
{
    m_width = width;
    m_height = height;
    m_left = -width * 0.5f;
    m_right = width * 0.5f;
    m_bottom = -height * 0.5f;
    m_top = height * 0.5f;
    m_nearPlane = nearPlane;
    m_farPlane = farPlane;
    m_isOrthoDirty = true;
}

void Camera2D::SetOrthographicProjection(float left, float right, float bottom, float top, float nearPlane, float farPlane)
{
    m_left = left;
    m_right = right;
    m_bottom = bottom;
    m_top = top;
    m_nearPlane = nearPlane;
    m_farPlane = farPlane;
    m_isOrthoDirty = true;
}

PrismaMath::mat4 Camera2D::GetViewMatrix() const
{
    if (m_isViewDirty)
    {
        // 计算视图矩阵: 相机的变换矩阵的逆矩阵
        // 注意顺序：先旋转，再平移
        PrismaMath::mat4 translationMatrix = Prisma::Math::Translation(PrismaMath::vec3(m_position));
        PrismaMath::mat4 rotationMatrix = Prisma::Math::RotationZ(m_rotation);

        // 组合相机的变换矩阵：平移 * 旋转
        PrismaMath::mat4 cameraMatrix = Prisma::Math::Multiply(translationMatrix, rotationMatrix);

        // 视图矩阵是相机变换矩阵的逆矩阵
        m_viewMatrix = Prisma::Math::Inverse(cameraMatrix);
        m_isViewDirty = false;
    }

    return m_viewMatrix;
}

PrismaMath::mat4 Camera2D::GetProjectionMatrix() const
{
    if (m_isOrthoDirty)
    {
        m_projectionMatrix = Prisma::Math::Orthographic(m_left, m_right, m_bottom, m_top, m_nearPlane, m_farPlane);
        m_isOrthoDirty = false;
    }
    
    return m_projectionMatrix;
}

PrismaMath::mat4 Camera2D::GetViewProjectionMatrix() const
{
    return Prisma::Math::Multiply(GetViewMatrix(), GetProjectionMatrix());
}

void Camera2D::UpdateProjectionMatrix(float windowWidth, float windowHeight)
{
    // 计算宽高比
    float aspectRatio = windowWidth / windowHeight;

    // 保持高度为2个单位，根据宽高比计算宽度
    float viewHeight = 2.0f;
    float viewWidth = viewHeight * aspectRatio;

    // 更新投影参数
    m_width = viewWidth;
    m_height = viewHeight;
    m_left = -viewWidth * 0.5f;
    m_right = viewWidth * 0.5f;
    m_bottom = -viewHeight * 0.5f;
    m_top = viewHeight * 0.5f;

    // 标记投影矩阵需要重新计算
    m_isOrthoDirty = true;
}

void Camera2D::SetNearFarPlanes(float nearPlane, float farPlane)
{
    m_nearPlane = nearPlane;
    m_farPlane = farPlane;
    m_isOrthoDirty = true;
}

void Camera2D::SetAspectRatio(float aspectRatio)
{
    // 保持高度为2个单位，根据宽高比计算宽度
    float viewHeight = 2.0f;
    float viewWidth = viewHeight * aspectRatio;

    // 更新投影参数
    m_width = viewWidth;
    m_height = viewHeight;
    m_left = -viewWidth * 0.5f;
    m_right = viewWidth * 0.5f;
    m_bottom = -viewHeight * 0.5f;
    m_top = viewHeight * 0.5f;

    // 标记投影矩阵需要重新计算
    m_isOrthoDirty = true;
}