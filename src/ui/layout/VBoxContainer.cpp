//
// 垂直布局：按子节点顺序从上到下排列，每个子节点保持自身尺寸，仅设置 position
//

#include "VBoxContainer.h"
#include "base/Transform.h"
#include "FrameState.h"

namespace morrow {

VBoxContainer::VBoxContainer()
    : UIWidget(false) {
    m_widgetType = "VBoxContainer";
}

void VBoxContainer::setSpacing(float spacing) {
    if (m_spacing != spacing) {
        m_spacing = spacing;
        requestRender("setSpacing");
    }
}

void VBoxContainer::update(FrameStateSharedPtr frameState) {
    layoutChildren();
    Widget::update(frameState);
}

void VBoxContainer::layoutChildren() {
    float cursorY = 0.0f;
    for (const auto& child : m_children) {
        if (!child->getVisible()) continue;
        auto childTransform = child->getComponent<Transform>();
        if (!childTransform) continue;

        Vector3 childSize = childTransform->getSize();
        childTransform->setPosition(0.0f, cursorY, 0.0f);
        cursorY += childSize.y + m_spacing;
    }
}

} // morrow
