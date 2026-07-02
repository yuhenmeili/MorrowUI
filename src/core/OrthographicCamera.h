//
// Created by lance on 2022/10/23.
//

#ifndef MORROW_ORTHOGRAPHIC_CAMERA_H
#define MORROW_ORTHOGRAPHIC_CAMERA_H

#include <memory>
#include "Camera.h"

namespace morrow
{
class OrthographicCamera : public Camera
{
public:
    OrthographicCamera(float near, float far);

    virtual ~OrthographicCamera()=default;

    void update(float x, float y, float displayWidth, float displayHeight) override;

private:
    float zoom = 1.0f;
};

using OrthographicCameraSharedPtr = std::shared_ptr<OrthographicCamera>;
}


#endif //MORROW_ORTHOGRAPHIC_CAMERA_H
