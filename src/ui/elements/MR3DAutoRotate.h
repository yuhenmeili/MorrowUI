//
// Created by 0060328 on 2026/4/20.
//

#ifndef MORROW_GUI_MRSCENEROOTAUTOROTATEDRIVER_H
#define MORROW_GUI_MRSCENEROOTAUTOROTATEDRIVER_H
#include "base/Component.h"
#include "base/Transform3D.h"


namespace morrow {
constexpr float kDegreesToRadians = 3.14159265358979323846f / 180.0f;

class MR3DAutoRotate final : public Component {
public:
    void setup(const Transform3DSharedPtr& transform, float degreesPerFrame = 1.0f);

    void update(FrameStateSharedPtr /*frameState*/) override;

    bool requiresContinuousUpdate() const override;

private:
    Transform3DSharedPtr m_transform;
    float m_degreesPerFrame = 1.0f;
    float m_angleRadians = 0.0f;
};
}


#endif //MORROW_GUI_MRSCENEROOTAUTOROTATEDRIVER_H
