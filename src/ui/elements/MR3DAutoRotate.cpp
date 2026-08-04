//
// Created by 0060328 on 2026/4/20.
//

#include "MR3DAutoRotate.h"

void morrow::MR3DAutoRotate::setup(const Transform3DSharedPtr& transform, float degreesPerFrame) {
    m_transform = transform;
    m_degreesPerFrame = degreesPerFrame;
}

void morrow::MR3DAutoRotate::update(FrameStateSharedPtr frame_state) {
    if (!m_transform) {
        return;
    }

    m_angleRadians += m_degreesPerFrame * kDegreesToRadians;
    m_transform->setLocalRotation(Quaternion::fromAxisAngle(Vector3(0.0f, 1.0f, 0.0f), m_angleRadians));
}

bool morrow::MR3DAutoRotate::requiresContinuousUpdate() const {
    return m_transform && m_degreesPerFrame != 0.0f;
}
