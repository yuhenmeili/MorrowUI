//
// Created by lance on 2022/10/23.
//

#include "Camera.h"
namespace morrow
{
const Matrix4& Camera::getProjectionView() const
{
    return m_projectionView;
}

Vector2 Camera::screenToWorld(float x, float y) const
{
    const float halfW = m_screenSize.x * 0.5f;
    const float halfH = m_screenSize.y * 0.5f;
    return Vector2(x - halfW, halfH - y);
}

Vector2 Camera::worldToScreen(float x, float y) const
{
    const float halfW = m_screenSize.x * 0.5f;
    const float halfH = m_screenSize.y * 0.5f;
    return Vector2(x + halfW, halfH - y);
}

const Vector2& Camera::getScreenSize() const
{
    return m_screenSize;
}

void Camera::setPosition(Vector3& position)
{
    m_position.copy(position);
}

void Camera::setPosition(float x, float y, float z)
{
    m_position.set(x, y, z);
}

void Camera::setDirection(Vector3& direction)
{
    m_direction.copy(direction);
}

void Camera::setDirection(float x, float y, float z)
{
    m_direction.set(x, y, z);
}

void Camera::setUp(Vector3& up)
{
    m_up.copy(up);
}

void Camera::setUp(float x, float y, float z)
{
    m_up.set(x, y, z);
}

const Vector3& Camera::getPosition() const
{
    return m_position;
}

const Vector3& Camera::getDirection() const
{
    return m_direction;
}

const Vector3& Camera::getUp() const
{
    return m_up;
}

}
