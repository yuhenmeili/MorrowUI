//
// Created by 0060328 on 2026/4/20.
//

#ifndef MORROW_GUI_MRCAMERAORBITPOSITIONDRIVER_H
#define MORROW_GUI_MRCAMERAORBITPOSITIONDRIVER_H
#include "base/Component.h"

namespace morrow {
class OrbitCamera;
using namespace Math;

class MR3DCameraOrbitPosition final : public Component {
public:
    void setup(OrbitCamera* camera,
               const Vector3& focus,
               float radius,
               float height,
               float angularSpeed,
               float startAngle);

    void update(FrameStateSharedPtr frameState) override;

private:
    OrbitCamera* m_camera = nullptr;
    Vector3 m_focus = {0.0f, 0.0f, 0.0f};
    float m_radius = 10.0f;
    float m_height = 3.0f;
    float m_angularSpeed = 0.6f;
    float m_angle = 0.0f;
};
} // morrow

#endif //MORROW_GUI_MRCAMERAORBITPOSITIONDRIVER_H
