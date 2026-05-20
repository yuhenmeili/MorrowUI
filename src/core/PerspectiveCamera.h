//
// Created by lance on 2023/5/11.
//

#ifndef MORROW_PERSPECTIVE_CAMERA_H
#define MORROW_PERSPECTIVE_CAMERA_H

#include "Camera.h"

namespace morrow
{

class PerspectiveCamera : public Camera
{
public:
    explicit PerspectiveCamera(float fov = 50.0f, float aspect = 1.0f, float near = 0.1f, float far = 2000.0f);

    void update(float x, float y, float displayWidth, float displayHeight) override;

    [[nodiscard]] float getFov() const { return m_fov; }

private:
    float m_fov = 50.0f;
    float m_aspect = 1.0f;

    float m_zoom = 1.0f;
    float m_focus = 10.0f;
};

} // MORROWGUI

#endif //MORROW_PERSPECTIVE_CAMERA_H
