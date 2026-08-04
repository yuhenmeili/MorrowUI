//
// 内边距布局：将第一个可见子节点放在 (left, top)，并将其尺寸设为容器尺寸减去四周边距
//

#include "MarginContainer.h"
#include "base/Transform.h"
#include "FrameState.h"

namespace morrow {

MarginContainer::MarginContainer()
    : UIWidget(false) {
    setWidgetType("MarginContainer");
}

void MarginContainer::setMarginLeft(float value) {
    if (m_marginLeft != value) {
        m_marginLeft = value;
        requestRender("setMarginLeft");
    }
}

void MarginContainer::setMarginTop(float value) {
    if (m_marginTop != value) {
        m_marginTop = value;
        requestRender("setMarginTop");
    }
}

void MarginContainer::setMarginRight(float value) {
    if (m_marginRight != value) {
        m_marginRight = value;
        requestRender("setMarginRight");
    }
}

void MarginContainer::setMarginBottom(float value) {
    if (m_marginBottom != value) {
        m_marginBottom = value;
        requestRender("setMarginBottom");
    }
}

void MarginContainer::setMargin(float left, float top, float right, float bottom) {
    m_marginLeft = left;
    m_marginTop = top;
    m_marginRight = right;
    m_marginBottom = bottom;
    requestRender("setMargin");
}

void MarginContainer::setMarginAll(float value) {
    m_marginLeft = m_marginTop = m_marginRight = m_marginBottom = value;
    requestRender("setMarginAll");
}

void MarginContainer::update(FrameStateSharedPtr frameState) {
    layoutChildren();
    Widget::update(frameState);
}

void MarginContainer::layoutChildren() {
    auto selfTransform = getComponent<Transform>();
    if (!selfTransform) return;

    Vector3 containerSize = selfTransform->getSize();
    float innerW = containerSize.x - m_marginLeft - m_marginRight;
    float innerH = containerSize.y - m_marginTop - m_marginBottom;
    if (innerW < 0.0f) innerW = 0.0f;
    if (innerH < 0.0f) innerH = 0.0f;

    for (const auto& child : m_children) {
        if (!child->getVisible()) continue;
        auto childTransform = child->getComponent<Transform>();
        if (!childTransform) continue;

        childTransform->setPosition(m_marginLeft, m_marginTop, 0.0f);
        childTransform->setSize(innerW, innerH);
        break; // 仅对第一个可见子节点应用内边距
    }
}

} // morrow
