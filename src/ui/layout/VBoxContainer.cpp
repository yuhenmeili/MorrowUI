//
// 垂直布局：按子节点顺序从上到下排列，每个子节点保持自身尺寸，仅设置 position
//

#include "VBoxContainer.h"

#include <algorithm>

#include "FrameState.h"
#include "base/Transform.h"
#include "elements/MRSpacer.h"

namespace morrow {

VBoxContainer::VBoxContainer() : UIWidget(false) {
    setWidgetType("VBoxContainer");
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
    size_t visibleCount = 0;
    float fixedHeight = 0.0f;
    float spacerMinimumHeight = 0.0f;
    float totalFlex = 0.0f;
    for (const auto& child : m_children) {
        if (!child->getVisible())
            continue;
        ++visibleCount;
        if (auto spacer = std::dynamic_pointer_cast<MRSpacer>(child)) {
            spacerMinimumHeight += spacer->getMinimumSize().y;
            totalFlex += spacer->getFlex();
        } else if (auto transform = child->getComponent<Transform>()) {
            fixedHeight += transform->getSize().y;
        }
    }

    const float spacingHeight = visibleCount > 0 ? static_cast<float>(visibleCount - 1) * m_spacing : 0.0f;
    const float availableHeight = getComponent<Transform>()->getSize().y;
    const float flexibleHeight = std::max(0.0f, availableHeight - fixedHeight - spacerMinimumHeight - spacingHeight);

    float cursorY = 0.0f;
    for (const auto& child : m_children) {
        if (!child->getVisible())
            continue;
        auto childTransform = child->getComponent<Transform>();
        if (!childTransform)
            continue;

        if (auto spacer = std::dynamic_pointer_cast<MRSpacer>(child)) {
            const Vector2 minimum = spacer->getMinimumSize();
            const float share = totalFlex > 0.0f ? flexibleHeight * spacer->getFlex() / totalFlex : 0.0f;
            childTransform->setSize(minimum.x, minimum.y + share);
        }
        const Vector3 childSize = childTransform->getSize();
        childTransform->setPosition(0.0f, cursorY, 0.0f);
        cursorY += childSize.y + m_spacing;
    }
}

}  // namespace morrow
