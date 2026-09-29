//
// Created by lance on 2022/10/23.
//

#ifndef MORROW_CAMERA_H
#define MORROW_CAMERA_H

#include "Vector3.h"
#include "Matrix4.h"

namespace morrow {
using namespace Math;

class Camera {
public:
    virtual void update(float x, float y, float displayWidth, float displayHeight) = 0;

    const Matrix4& getProjectionView() const;

    Vector2 screenToWorld(float x, float y) const;

    Vector2 worldToScreen(float x, float y) const;

    const Vector2& getScreenSize() const;

    void setPosition(Vector3& position);

    void setPosition(float x, float y, float z);

    void setDirection(Vector3& direction);

    void setDirection(float x, float y, float z);

    void setUp(Vector3& up);

    void setUp(float x, float y, float z);

    const Vector3& getPosition() const;

    const Vector3& getDirection() const;

    const Vector3& getUp() const;

protected:
    Vector3 m_position;
    Vector3 m_direction = {0.0f, 0.0f, -1.0f};
    Vector3 m_up = {0.0f, 1.0f, 0.0f};
    Matrix4 m_matrixWorld;
    Matrix4 m_projection;
    Matrix4 m_view;
    Matrix4 m_projectionView;
    float m_near = 0.0f;
    float m_far = 2000.0f;

    Vector2 m_screenPos;
    Vector2 m_screenSize;
};
}


#endif //MORROW_CAMERA_H
