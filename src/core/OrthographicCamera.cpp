//
// Created by lance on 2022/10/23.
//

#include "OrthographicCamera.h"

namespace morrow
{
OrthographicCamera::OrthographicCamera(float near, float far)
{
    m_near = near;
    m_far = far;
}

void OrthographicCamera::update(float x, float y, float displayWidth, float displayHeight)
{
    m_screenPos.x = x;
    m_screenPos.y = y;
    m_screenSize.x = displayWidth;
    m_screenSize.y = displayHeight;

    Vector3 tmp;
    const float halfW = displayWidth * 0.5f;
    const float halfH = displayHeight * 0.5f;
    const float left = -halfW;
    const float right = halfW;
    const float top = halfH;
    const float bottom = -halfH;

    m_projection.makeOrthographic(left, right, top, bottom, m_near, m_far);
    m_view.setToLookAt(m_position, tmp.copy(m_position).add(m_direction), m_up);
    m_projectionView.copy(m_projection);
    m_projectionView.multiply(m_view);
}

}
