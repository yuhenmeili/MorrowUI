//
// Created by lance on 2025/10/3.
//

#include "UIWidget.h"
#include "base/Transform.h"
#include "FrameState.h"

#include <algorithm>
#include <array>
#include <limits>

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
    const auto transform = getTransform();
    if (!transform)
        return Math::Rect(0.0f, 0.0f, 0.0f, 0.0f);

    // TouchEvent positions use framebuffer pixels with a top-left origin and
    // Y increasing downwards. UI world space is centered with Y increasing
    // upwards, so derive the screen extent from the root transform and map
    // every transformed corner into the input coordinate system here.
    const Widget* root = this;
    while (root->m_parent) {
        root = root->m_parent.get();
    }

    Vector3 screenSize;
    if (const auto rootTransform = root->getComponent<Transform>()) {
        screenSize = rootTransform->getSize();
    }

    const Matrix4& worldMatrix = transform->getWorldMatrix();
    const Vector3 size = transform->getSize();
    const float halfW = size.x * 0.5f;
    const float halfH = size.y * 0.5f;
    std::array<Vector3, 4> corners = {
        Vector3(-halfW, -halfH, 0.0f),
        Vector3(-halfW, halfH, 0.0f),
        Vector3(halfW, -halfH, 0.0f),
        Vector3(halfW, halfH, 0.0f),
    };

    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();
    const float halfScreenW = screenSize.x * 0.5f;
    const float halfScreenH = screenSize.y * 0.5f;

    for (auto& corner : corners) {
        corner.apply(worldMatrix);
        const float screenX = corner.x + halfScreenW;
        const float screenY = halfScreenH - corner.y;
        minX = std::min(minX, screenX);
        minY = std::min(minY, screenY);
        maxX = std::max(maxX, screenX);
        maxY = std::max(maxY, screenY);
    }

    return Math::Rect(minX, minY, maxX, maxY);
}

void UIWidget::setClipChildren(bool clip) {
    if (m_clipChildren == clip)
        return;
    m_clipChildren = clip;
    requestRender("setClipChildren");
}

bool UIWidget::getClipChildren() const {
    return m_clipChildren;
}

void UIWidget::update(FrameStateSharedPtr frameState) {
    m_componentManager->updateComponents(frameState);

    ClipRect previousClip;
    bool pushedClip = false;
    if (frameState && m_clipChildren) {
        previousClip = frameState->currentClip;
        frameState->clipStack.push_back(previousClip);
        const Math::Rect bounds = getScreenSpaceAABB();
        frameState->currentClip = previousClip.intersected(
            ClipRect::fromBounds(
                bounds.Min.x, bounds.Min.y, bounds.Max.x, bounds.Max.y));
        pushedClip = true;
    }

    if (!frameState || !frameState->currentClip.empty()) {
        for (auto& child : m_children) {
            if (child->getVisible()) {
                child->update(frameState);
            }
        }
    }

    if (pushedClip) {
        frameState->currentClip = frameState->clipStack.back();
        frameState->clipStack.pop_back();
    }
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
