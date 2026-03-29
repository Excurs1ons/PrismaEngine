#include "OrthographicCamera.h"

namespace Prisma {
namespace Graphic {

OrthographicCamera::OrthographicCamera(float left, float right, float bottom, float top, float nearPlane, float farPlane)
    : m_left(left), m_right(right), m_bottom(bottom), m_top(top), m_near(nearPlane), m_far(farPlane)
{
    RecalculateMatrices();
}

void OrthographicCamera::Update(Timestep /*ts*/) {
    // 基础正交相机不做每帧逻辑
}

void OrthographicCamera::SetProjection(float left, float right, float bottom, float top) {
    m_left = left;
    m_right = right;
    m_bottom = bottom;
    m_top = top;
    RecalculateMatrices();
}

void OrthographicCamera::RecalculateMatrices() {
    // 计算投影矩阵 (使用 GLM 的 orthoRH)
    // 注意：缩放通过调整左右上下边界来实现，或者直接作用于投影矩阵
    float width = (m_right - m_left) / m_zoom;
    float height = (m_top - m_bottom) / m_zoom;
    float centerX = (m_left + m_right) * 0.5f;
    float centerY = (m_bottom + m_top) * 0.5f;

    m_projectionMatrix = glm::orthoRH(centerX - width * 0.5f, centerX + width * 0.5f, 
                                      centerY - height * 0.5f, centerY + height * 0.5f, 
                                      m_near, m_far);

    // 计算视图矩阵 (取反移动)
    m_viewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-m_position.x, -m_position.y, 0.0f));
    
    // 如果有旋转
    if (m_rotation != 0.0f) {
        m_viewMatrix = glm::rotate(m_viewMatrix, glm::radians(m_rotation), glm::vec3(0.0f, 0.0f, 1.0f));
    }

    m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
}

} // namespace Graphic
} // namespace Prisma
