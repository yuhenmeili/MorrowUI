//
// 居中布局：以第一个可见子节点为准，将其左上角放置在容器中心偏左上（使子节点几何中心与容器中心对齐）
//

#include "CenterContainer.h"
#include "base/Transform.h"
#include "FrameState.h"

namespace morrow {

CenterContainer::CenterContainer()
    : UIWidget(false) {
    m_widgetType = "CenterContainer";
}

void CenterContainer::update(FrameStateSharedPtr frameState) {
    layoutChildren();
    Widget::update(frameState);
}

void CenterContainer::layoutChildren() {
    auto selfTransform = getComponent<Transform>();
    if (!selfTransform) return;

    Vector3 containerSize = selfTransform->getSize();
    bool first = true;
    for (const auto& child : m_children) {
        if (!child->getVisible()) continue;
        auto childTransform = child->getComponent<Transform>();
        if (!childTransform) continue;
        // 通常只居中第一个子节点；若需全部居中可去掉 first 逻辑
        if (!first) break;
        first = false;

        Vector3 childSize = childTransform->getSize();
        float x = (containerSize.x - childSize.x) * 0.5f;
        float y = (containerSize.y - childSize.y) * 0.5f;
        childTransform->setPosition(x, y, 0.0f);
    }
}

} // morrow
