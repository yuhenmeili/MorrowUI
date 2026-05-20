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
    m_matrixDirty = true;
}

void Transform3D::setLocalPosition(const Vector3& position) {
    switchToTRSMode();
    m_localPosition = position;
    m_matrixDirty = true;
}

Vector3 Transform3D::getLocalPosition() const {
    return m_localPosition;
}

void Transform3D::setLocalScale(float x, float y, float z) {
    switchToTRSMode();
    m_localScale.set(x, y, z);
    m_matrixDirty = true;
}

void Transform3D::setLocalScale(const Vector3& scale) {
    switchToTRSMode();
    m_localScale = scale;
    m_matrixDirty = true;
}

Vector3 Transform3D::getLocalScale() const {
    return m_localScale;
}

void Transform3D::setLocalRotation(float x, float y, float z, float w) {
    switchToTRSMode();
    m_localRotation.set(x, y, z, w);
    m_matrixDirty = true;
}

void Transform3D::setLocalRotation(const Quaternion& rotation) {
    switchToTRSMode();
    m_localRotation = rotation;
    m_matrixDirty = true;
}

Quaternion Transform3D::getLocalRotation() const {
    return m_localRotation;
}

void Transform3D::setLocalTransformMatrix(const Matrix4& matrix) {
    m_localMatrix = matrix;
    m_localMatrix.decompose(m_localPosition, m_localRotation, m_localScale);
    m_useExplicitLocalMatrix = true;
    m_matrixDirty = false;
}

std::shared_ptr<Transform3D> Transform3D::getParentTransform() const {
    return const_cast<Transform3D*>(this)->getComponentInParent<Transform3D>();
}

bool Transform3D::hasParentTransform() const {
    return static_cast<bool>(getParentTransform());
}

const Matrix4& Transform3D::getLocalTransformMatrix() {
    if (m_matrixDirty) {
        updateMatrix();
    }
    return m_localMatrix;
}

const Matrix4& Transform3D::getWorldTransformMatrix() {
    if (m_matrixDirty) {
        updateMatrix();
    }

    auto parentTransform = getParentTransform();
    if (parentTransform) {
        m_worldMatrix.multiplyMatrices(parentTransform->getWorldTransformMatrix(), m_localMatrix);
    } else {
        m_worldMatrix = m_localMatrix;
    }

    return m_worldMatrix;
}

const Matrix4& Transform3D::getSceneTransformMatrix() {
    return getWorldTransformMatrix();
}

void Transform3D::update(FrameStateSharedPtr /*frameState*/) {
    if (m_matrixDirty) {
        updateMatrix();
    }
}

void Transform3D::setDirty() {
    m_matrixDirty = true;
}

void Transform3D::switchToTRSMode() {
    if (!m_useExplicitLocalMatrix) {
        return;
    }
    m_useExplicitLocalMatrix = false;
}

void Transform3D::updateMatrix() {
    if (m_useExplicitLocalMatrix) {
        m_matrixDirty = false;
        return;
    }

    m_localMatrix.compose(m_localPosition, m_localRotation, m_localScale);
    m_matrixDirty = false;
}
} // namespace morrow
