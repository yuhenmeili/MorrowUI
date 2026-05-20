#ifndef MORROW_TRANSFORM3D_H
#define MORROW_TRANSFORM3D_H

#include "Component.h"
#include "Vector3.h"
#include "Quaternion.h"
#include "Matrix4.h"

namespace morrow {
using namespace Math;

// 3D transform without UI anchor/pivot/screen-space adjustments.
class Transform3D : public Component {
public:
    Transform3D();

    ~Transform3D() override = default;

    void setLocalPosition(float x, float y, float z);

    void setLocalPosition(const Vector3& position);

    Vector3 getLocalPosition() const;

    void setLocalScale(float x, float y, float z);

    void setLocalScale(const Vector3& scale);

    Vector3 getLocalScale() const;

    void setLocalRotation(float x, float y, float z, float w);

    void setLocalRotation(const Quaternion& rotation);

    Quaternion getLocalRotation() const;

    void setLocalTransformMatrix(const Matrix4& matrix);

    std::shared_ptr<Transform3D> getParentTransform() const;

    bool hasParentTransform() const;

    const Matrix4& getLocalTransformMatrix();

    const Matrix4& getWorldTransformMatrix();

    const Matrix4& getSceneTransformMatrix();

    void update(FrameStateSharedPtr frameState) override;

    void setDirty();

private:
    void switchToTRSMode();

    void updateMatrix();

private:
    Vector3 m_localPosition;
    Vector3 m_localScale;
    Quaternion m_localRotation;

    Matrix4 m_localMatrix;
    Matrix4 m_worldMatrix;
    bool m_useExplicitLocalMatrix = false;
    bool m_matrixDirty = true;
};

using Transform3DSharedPtr = std::shared_ptr<Transform3D>;
} // namespace morrow

#endif // MORROW_TRANSFORM3D_H
