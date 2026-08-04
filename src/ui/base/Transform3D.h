#ifndef MORROW_TRANSFORM3D_H
#define MORROW_TRANSFORM3D_H

#include <cstdint>

#include "Component.h"
#include "Matrix4.h"
#include "Quaternion.h"
#include "Vector3.h"

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

    /// Incremented whenever the cached world matrix is actually rebuilt.
    uint64_t getWorldVersion() const {
        return m_worldVersion;
    }

    /// Incremented whenever local transform data changes.
    uint64_t getLocalVersion() const {
        return m_localVersion;
    }

    void update(FrameStateSharedPtr frameState) override;

    void setDirty();

private:
    void switchToTRSMode();

    void markDirty();

    void updateMatrix();

private:
    Vector3 m_localPosition;
    Vector3 m_localScale;
    Quaternion m_localRotation;

    Matrix4 m_localMatrix;
    Matrix4 m_worldMatrix;
    bool m_useExplicitLocalMatrix = false;

    /// Local transform data revision.
    uint64_t m_localVersion = 1;
    /// Local data revision used to build m_localMatrix.
    uint64_t m_localMatrixVersion = 0;
    /// Local matrix revision used to build m_worldMatrix.
    uint64_t m_worldLocalMatrixVersion = 0;
    /// Incremented whenever m_worldMatrix is actually rebuilt.
    uint64_t m_worldVersion = 1;
    /// Parent identity and world revision used to build m_worldMatrix.
    Transform3D* m_cachedWorldParent = nullptr;
    uint64_t m_cachedParentWorldVersion = 0;
};

using Transform3DSharedPtr = std::shared_ptr<Transform3D>;
}  // namespace morrow

#endif  // MORROW_TRANSFORM3D_H
