#include "Transform3D.h"

#include "Component.inl"

namespace morrow {
Transform3D::Transform3D()
    : m_localPosition(0.0f, 0.0f, 0.0f)
      , m_localScale(1.0f, 1.0f, 1.0f)
      , m_localRotation(0.0f, 0.0f, 0.0f, 1.0f)
      , m_localMatrix(Matrix4())
      , m_worldMatrix(Matrix4()) {
}

void Transform3D::setLocalPosition(float x, float y, float z) {
    switchToTRSMode();
    m_localPosition.set(x, y, z);
    markDirty();
}

void Transform3D::setLocalPosition(const Vector3& position) {
    switchToTRSMode();
    m_localPosition = position;
    markDirty();
}

Vector3 Transform3D::getLocalPosition() const {
    return m_localPosition;
}

void Transform3D::setLocalScale(float x, float y, float z) {
    switchToTRSMode();
    m_localScale.set(x, y, z);
    markDirty();
}

void Transform3D::setLocalScale(const Vector3& scale) {
    switchToTRSMode();
    m_localScale = scale;
    markDirty();
}

Vector3 Transform3D::getLocalScale() const {
    return m_localScale;
}

void Transform3D::setLocalRotation(float x, float y, float z, float w) {
    switchToTRSMode();
    m_localRotation.set(x, y, z, w);
    markDirty();
}

void Transform3D::setLocalRotation(const Quaternion& rotation) {
    switchToTRSMode();
    m_localRotation = rotation;
    markDirty();
}

Quaternion Transform3D::getLocalRotation() const {
    return m_localRotation;
}

void Transform3D::setLocalTransformMatrix(const Matrix4& matrix) {
    m_localMatrix = matrix;
    m_localMatrix.decompose(m_localPosition, m_localRotation, m_localScale);
    m_useExplicitLocalMatrix = true;
    markDirty();
    m_localMatrixVersion = m_localVersion;
}

std::shared_ptr<Transform3D> Transform3D::getParentTransform() const {
    return const_cast<Transform3D*>(this)->getComponentInParent<Transform3D>();
}

bool Transform3D::hasParentTransform() const {
    return static_cast<bool>(getParentTransform());
}

const Matrix4& Transform3D::getLocalTransformMatrix() {
    if (m_localMatrixVersion != m_localVersion) {
        updateMatrix();
    }
    return m_localMatrix;
}

const Matrix4& Transform3D::getWorldTransformMatrix() {
    auto parentTransform = getParentTransform();
    Transform3D* const parent = parentTransform.get();
    const Matrix4* parentWorldMatrix = nullptr;
    uint64_t parentWorldVersion = 0;

    // Refresh the parent first so ancestor changes propagate through an
    // otherwise unqueried transform chain.
    if (parentTransform) {
        parentWorldMatrix = &parentTransform->getWorldTransformMatrix();
        parentWorldVersion = parentTransform->getWorldVersion();
    }

    const bool parentChanged =
        m_cachedWorldParent != parent ||
        m_cachedParentWorldVersion != parentWorldVersion;

    const Matrix4& localMatrix = getLocalTransformMatrix();
    if (m_worldLocalMatrixVersion != m_localMatrixVersion || parentChanged) {
        if (parentWorldMatrix) {
            m_worldMatrix.multiplyMatrices(*parentWorldMatrix, localMatrix);
        } else {
            m_worldMatrix = localMatrix;
        }

        m_worldLocalMatrixVersion = m_localMatrixVersion;
        m_cachedWorldParent = parent;
        m_cachedParentWorldVersion = parentWorldVersion;
        ++m_worldVersion;
    }

    return m_worldMatrix;
}

const Matrix4& Transform3D::getSceneTransformMatrix() {
    return getWorldTransformMatrix();
}

void Transform3D::update(FrameStateSharedPtr /*frameState*/) {
    if (m_localMatrixVersion != m_localVersion) {
        updateMatrix();
    }
}

void Transform3D::setDirty() {
    markDirty();
}

void Transform3D::switchToTRSMode() {
    if (!m_useExplicitLocalMatrix) {
        return;
    }
    m_useExplicitLocalMatrix = false;
}

void Transform3D::markDirty() {
    ++m_localVersion;
}

void Transform3D::updateMatrix() {
    if (m_useExplicitLocalMatrix) {
        m_localMatrixVersion = m_localVersion;
        return;
    }

    m_localMatrix.compose(m_localPosition, m_localRotation, m_localScale);
    m_localMatrixVersion = m_localVersion;
}
} // namespace morrow
