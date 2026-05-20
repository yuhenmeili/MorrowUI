//
// Created by 0060328 on 2026/4/20.
//

#include "MR3DCameraOrbitPosition.h"

#include "OrbitCamera.h"

namespace morrow {
void MR3DCameraOrbitPosition::setup(OrbitCamera* camera, const Vector3& focus, float radius, float height, float angularSpeed, float startAngle) {
    m_camera = camera;
    m_focus = focus;
    m_radius = std::max(radius, 0.001f);
    m_height = height;
    m_angularSpeed = angularSpeed;
    m_angle = startAngle;
}

void MR3DCameraOrbitPosition::update(FrameStateSharedPtr frameState) {
    if (!m_camera) {
        return;
    }

    const float dt = frameState ? float(frameState->deltaTime) : (1.0f / 60.0f);
    m_angle += m_angularSpeed * dt;
    m_camera->setPosition(
        m_focus.x + std::sin(m_angle) * m_radius,
        m_focus.y + m_height,
        m_focus.z + std::cos(m_angle) * m_radius);
}
} // morrow