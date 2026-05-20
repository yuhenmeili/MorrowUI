//
// 水平布局：按子节点顺序从左到右排列，每个子节点保持自身尺寸，仅设置 position
//

#include "HBoxContainer.h"
#include "base/Transform.h"
#include "FrameState.h"

namespace morrow {

HBoxContainer::HBoxContainer()
    : UIWidget(false) {
    m_widgetType = "HBoxContainer";
}

void HBoxContainer::setSpacing(float spacing) {
    if (m_spacing != spacing) {
        m_spacing = spacing;
        requestRender("setSpacing");
    }
}

void HBoxContainer::update(FrameStateSharedPtr frameState) {
    layoutChildren();
    Widget::update(frameState);
}

void HBoxContainer::layoutChildren() {
    float cursorX = 0.0f;
    for (const auto& child : m_children) {
        if (!child->getVisible()) continue;
        auto childTransform = child->getComponent<Transform>();
        if (!childTransform) continue;

        Vector3 childSize = childTransform->getSize();
        childTransform->setPosition(cursorX, 0.0f, 0.0f);
        cursorX += childSize.x + m_spacing;
    }
}

} // morrow
