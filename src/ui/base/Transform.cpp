#include "Transform.h"
#include "Widget.h"
#include <cmath>
#include "Component.inl"
#include "GlobalObject.h"

namespace morrow {
Transform::Transform()
    : m_localPosition(0.0f, 0.0f, 0.0f)
      , m_localScale(1.0f, 1.0f, 1.0f)
      , m_size(0.0f, 0.0f, 0.0f)
      , m_localRotation(0.0f, 0.0f, 0.0f, 1.0f)
      , m_localMatrix(Matrix4())
      , m_worldMatrix(Matrix4())
      , m_matrixDirty(true) {
}

void Transform::setPosition(float x, float y, float z) {
    m_localPosition.set(x, y, z);
    m_matrixDirty = true;
}

void Transform::setPosition(const Vector3& position) {
    m_localPosition = position;
    m_matrixDirty = true;
}

Vector3 Transform::getPosition() const {
    return m_localPosition;
}

void Transform::setSize(float width, float height) {
    m_size.set(width, height, 0.0f);
    setDirty();
    notifySizeChange();
}

void Transform::setSize(const Vector3& size) {
    m_size = size;
    setDirty();
    notifySizeChange();
}

Vector3 Transform::getSize() const {
    return m_size;
}

Vector3 Transform::getCenter() const {
    return Vector3(0.0f, 0.0f, 0.0f);
}

void Transform::setScale(float x, float y, float z) {
    m_localScale.set(x, y, z);
    m_matrixDirty = true;
}

void Transform::setScale(const Vector3& scale) {
    m_localScale = scale;
    m_matrixDirty = true;
}

Vector3 Transform::getScale() const {
    return m_localScale;
}

void Transform::setRotation(float x, float y, float z, float w) {
    m_localRotation.set(x, y, z, w);
    m_matrixDirty = true;
}

void Transform::setRotation(const Quaternion& rotation) {
    m_localRotation = rotation;
    m_matrixDirty = true;
}

void Transform::setRotation(const Vector3& axis, float angle) {
    m_localRotation.setFromAxisAngle(axis, angle);
    m_matrixDirty = true;
}

Quaternion Transform::getRotation() const {
    return m_localRotation;
}

void Transform::setPivot(const Vector3& pivot) {
    m_pivotScreen = pivot;
    m_matrixDirty = true;
}

void Transform::setPivot(float x, float y, float z) {
    m_pivotScreen.set(x, y, z);
    m_matrixDirty = true;
}

Vector3 Transform::getPivot() const {
    return m_pivotScreen;
}

void Transform::setAnchor(const Anchor& anchor) {
    m_anchor = anchor;
}

void Transform::setAnchor(float minX, float minY, float maxX, float maxY) {
    m_anchor.min.set(minX, minY);
    m_anchor.max.set(maxX, maxY);
}

Anchor Transform::getAnchor() const {
    return m_anchor;
}

const Matrix4& Transform::getLocalMatrix() {
    if (m_matrixDirty) {
        updateMatrix();
    }
    return m_localMatrix;
}

const Matrix4& Transform::getWorldMatrix() {
    if (m_matrixDirty) {
        updateMatrix();
    }

    auto parentTransform = getComponentInParent<Transform>();
    if (parentTransform) {
        m_worldMatrix.multiplyMatrices(parentTransform->getWorldMatrix(), m_localMatrix);
    } else {
        m_worldMatrix = m_localMatrix;
    }

    return m_worldMatrix;
}

void Transform::update(FrameStateSharedPtr /*frameState*/) {
    if (m_matrixDirty) {
        updateMatrix();
    }
}

void Transform::addSizeChangeListener(const std::function<void()>& listener) {
    m_sizeChangeListeners.push_back(listener);
}

void Transform::notifySizeChange() {
    for (const auto& listener : m_sizeChangeListeners) {
        listener();
    }
}

void Transform::setDirty() {
    m_matrixDirty = true;
    for (auto& child : m_children) {
        child->setDirty();
    }
}

void Transform::updateMatrix() {
    auto parentTransform = getComponentInParent<Transform>();

    Vector3 renderPosition = m_localPosition;
    if (parentTransform) {
        const Vector3 parentSize = parentTransform->getSize();
        renderPosition.x = -parentSize.x * 0.5f + m_localPosition.x + m_size.x * 0.5f;
        renderPosition.y = parentSize.y * 0.5f - m_localPosition.y - m_size.y * 0.5f;
    }

    // Pivot input uses screen-style normalized coordinates:
    // (0,0)=top-left, (1,1)=bottom-right.
    const Vector3 pivotPoint(
        (m_pivotScreen.x - 0.5f) * m_size.x,
        (0.5f - m_pivotScreen.y) * m_size.y,
        m_pivotScreen.z
    );

    m_localMatrix.identity();
    m_localMatrix.applyTranslation(renderPosition.x, renderPosition.y, renderPosition.z);
    m_localMatrix.applyTranslation(pivotPoint.x, pivotPoint.y, pivotPoint.z);
    m_localMatrix.applyRotation(m_localRotation);
    m_localMatrix.applyScale(m_localScale.x, m_localScale.y, m_localScale.z);
    m_localMatrix.applyTranslation(-pivotPoint.x, -pivotPoint.y, -pivotPoint.z);

    m_matrixDirty = false;
    REQUESTRENDER;
}

void Transform::updateAnchoredPosition() {
    if (!m_parent) return;

    Vector3 parentSize = m_parent->getSize();

    Vector2 anchorMinWorld(
        m_anchor.min.x * parentSize.x,
        m_anchor.min.y * parentSize.y
    );
    Vector2 anchorMaxWorld(
        m_anchor.max.x * parentSize.x,
        m_anchor.max.y * parentSize.y
    );

    if (m_anchor.min.x == m_anchor.max.x && m_anchor.min.y == m_anchor.max.y) {
        m_localPosition = Vector3(
            anchorMinWorld.x + m_offset.x,
            anchorMinWorld.y + m_offset.y,
            m_localPosition.z
        );
    } else {
        m_localPosition = Vector3(
            anchorMinWorld.x + m_offset.x,
            anchorMinWorld.y + m_offset.y,
            m_localPosition.z
        );
        m_size = Vector3(
            (anchorMaxWorld.x - anchorMinWorld.x) - (m_offset.x + m_offset.z),
            (anchorMaxWorld.y - anchorMinWorld.y) - (m_offset.y + m_offset.w),
            m_size.z
        );
    }

    setDirty();
}

void Transform::updateFromParent() {
    if (!m_parent) {
        m_worldMatrix = m_localMatrix;
        return;
    }

    const Matrix4& parentWorldMatrix = m_parent->getWorldMatrix();

    if (m_anchor.min != m_anchor.max) {
        updateAnchoredPosition();
    }

    m_worldMatrix = parentWorldMatrix * m_localMatrix;

    for (auto& child : m_children) {
        child->setDirty();
    }
}
} // namespace morrow
