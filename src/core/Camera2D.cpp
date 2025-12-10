#include "Camera2D.h"

Camera2D::Camera2D()
    : m_position(XMVectorZero())
    , m_rotation(0.0f)
    , m_viewMatrix(XMMatrixIdentity())
    , m_projectionMatrix(XMMatrixIdentity())
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
    m_clearColor = XMVectorSet(0.0f, 1.0f, 1.0f, 1.0f);
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
    m_position = XMVectorSet(x, y, z, 0.0f);
    m_isViewDirty = true;
}

void Camera2D::SetPosition(FXMVECTOR position)
{
    m_position = position;
    m_isViewDirty = true;
}

XMVECTOR Camera2D::GetPosition() const
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

XMMATRIX Camera2D::GetViewMatrix() const
{
    if (m_isViewDirty)
    {
        // 计算视图矩阵: Rotation * Translation的逆矩阵
        XMMATRIX translationMatrix = XMMatrixTranslationFromVector(-m_position);
        XMMATRIX rotationMatrix = XMMatrixRotationZ(-m_rotation);
        
        m_viewMatrix = XMMatrixMultiply(translationMatrix, rotationMatrix);
        m_isViewDirty = false;
    }
    
    return m_viewMatrix;
}

XMMATRIX Camera2D::GetProjectionMatrix() const
{
    if (m_isOrthoDirty)
    {
        m_projectionMatrix = XMMatrixOrthographicOffCenterLH(m_left, m_right, m_bottom, m_top, m_nearPlane, m_farPlane);
        m_isOrthoDirty = false;
    }
    
    return m_projectionMatrix;
}

XMMATRIX Camera2D::GetViewProjectionMatrix() const
{
    return XMMatrixMultiply(GetViewMatrix(), GetProjectionMatrix());
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