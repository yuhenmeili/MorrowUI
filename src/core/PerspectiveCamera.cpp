//
// Created by lance on 2023/5/11.
//

#include "PerspectiveCamera.h"

namespace morrow
{

PerspectiveCamera::PerspectiveCamera(float fov, float aspect, float near, float far)
{
    m_fov = fov;
    m_aspect = aspect;
    m_near = near;
    m_far = far;
}

void PerspectiveCamera::update(float x, float y, float displayWidth, float displayHeight)
{
    m_screenPos.x = x;
    m_screenPos.y = y;
    m_screenSize.x = displayWidth;
    m_screenSize.y = displayHeight;

    m_aspect = displayWidth / displayHeight;

    Vector3 tmp;
    float top = m_near * std::tan(MR_PI / 180.0f * 0.5f * m_fov) / m_zoom;
    float height = 2 * top;
    float width = m_aspect * height;
    float left = -0.5f * width;

    m_projection.makePerspective(left, left + width, top, top - height, m_near, m_far);
    m_view.setToLookAt(m_position, tmp.copy(m_position).add(m_direction), m_up);
    m_projectionView.copy(m_projection);
    m_projectionView.multiply(m_view);
}
} // MORROWGUI
