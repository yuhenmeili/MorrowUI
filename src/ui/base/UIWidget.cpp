//
// Created by lance on 2025/10/3.
//

#include "UIWidget.h"
#include "base/Transform.h"

namespace morrow {
UIWidget::UIWidget(bool createRenderComponents)
    : Widget() {
    addComponent<Transform>();

    if (createRenderComponents) {
        m_meshFilter = addComponent<MeshFilter>();
        m_meshRenderer = addComponent<MeshRenderer>();
        m_material = m_meshRenderer->getMaterial();
        m_material->setFloat("alpha", 1.0f);
    }
}

std::shared_ptr<Transform> UIWidget::getTransform() {
    return getComponent<Transform>();
}

std::shared_ptr<Transform> UIWidget::getTransform() const {
    return getComponent<Transform>();
}

Math::Rect UIWidget::getScreenSpaceAABB() const {
    auto transform = getTransform();
    if (!transform) return Math::Rect(0.0f, 0.0f, 0.0f, 0.0f);

    const Matrix4& worldMatrix = transform->getWorldMatrix();
    const Vector3 worldPos(worldMatrix.elements[12], worldMatrix.elements[13], worldMatrix.elements[14]);
    const Vector3 size = transform->getSize();
    const float halfW = size.x * 0.5f;
    const float halfH = size.y * 0.5f;
    return Math::Rect(
        worldPos.x - halfW,
        worldPos.y - halfH,
        worldPos.x + halfW,
        worldPos.y + halfH
    );
}

void UIWidget::setAlpha(float alpha) {
    if (m_alpha != alpha) {
        m_alpha = alpha;
        if (m_material) {
            m_material->setFloat("alpha", alpha);
        }
        requestRender("setAlpha");
    }
}

void UIWidget::setUVData(const Vector2& uv0, const Vector2& uv1) {
    if (m_meshFilter) {
        m_meshFilter->setUVData(uv0, uv1);
    }
}

void UIWidget::initialize() {
}
} // morrow