//
// 水平布局：按子节点顺序从左到右排列，每个子节点保持自身尺寸，仅设置 position
//

#include "HBoxContainer.h"

#include <algorithm>

#include "FrameState.h"
#include "base/Transform.h"
#include "elements/MRSpacer.h"

namespace morrow {

HBoxContainer::HBoxContainer() : UIWidget(false) {
    setWidgetType("HBoxContainer");
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
    size_t visibleCount = 0;
    float fixedWidth = 0.0f;
    float spacerMinimumWidth = 0.0f;
    float totalFlex = 0.0f;
    for (const auto& child : m_children) {
        if (!child->getVisible())
            continue;
        ++visibleCount;
        if (auto spacer = std::dynamic_pointer_cast<MRSpacer>(child)) {
            spacerMinimumWidth += spacer->getMinimumSize().x;
            totalFlex += spacer->getFlex();
        } else if (auto transform = child->getComponent<Transform>()) {
            fixedWidth += transform->getSize().x;
        }
    }

    const float spacingWidth = visibleCount > 0 ? static_cast<float>(visibleCount - 1) * m_spacing : 0.0f;
    const float availableWidth = getComponent<Transform>()->getSize().x;
    const float flexibleWidth = std::max(0.0f, availableWidth - fixedWidth - spacerMinimumWidth - spacingWidth);

    float cursorX = 0.0f;
    for (const auto& child : m_children) {
        if (!child->getVisible())
            continue;
        auto childTransform = child->getComponent<Transform>();
        if (!childTransform)
            continue;

        if (auto spacer = std::dynamic_pointer_cast<MRSpacer>(child)) {
            const Vector2 minimum = spacer->getMinimumSize();
            const float share = totalFlex > 0.0f ? flexibleWidth * spacer->getFlex() / totalFlex : 0.0f;
            childTransform->setSize(minimum.x + share, minimum.y);
        }
        const Vector3 childSize = childTransform->getSize();
        childTransform->setPosition(cursorX, 0.0f, 0.0f);
        cursorX += childSize.x + m_spacing;
    }
}

}  // namespace morrow
