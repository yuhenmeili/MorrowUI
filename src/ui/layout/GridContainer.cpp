//
// 网格布局：子节点按 columns 列填入单元格；单元格均分容器尺寸减去间距，
// 行数默认按可见子节点数自动向上取整（rows <= 0 时）。
//

#include "morrow/layout/GridContainer.h"

#include <algorithm>

#include "morrow/scene3d/FrameState.h"
#include "morrow/base/Transform.h"

namespace morrow {

GridContainer::GridContainer() : UIWidget(false) {
    setWidgetType("GridContainer");
}

void GridContainer::setColumns(int columns) {
    columns = std::max(1, columns);
    if (m_columns != columns) {
        m_columns = columns;
        requestRender("setColumns");
    }
}

void GridContainer::setRows(int rows) {
    if (m_rows != rows) {
        m_rows = rows;
        requestRender("setRows");
    }
}

void GridContainer::setSpacing(float spacing) {
    if (m_spacing != spacing) {
        m_spacing = spacing;
        requestRender("setSpacing");
    }
}

void GridContainer::update(FrameStateSharedPtr frameState) {
    layoutChildren();
    Widget::update(frameState);
}

void GridContainer::layoutChildren() {
    size_t visibleCount = 0;
    for (const auto& child : m_children) {
        if (child->getVisible())
            ++visibleCount;
    }
    if (visibleCount == 0)
        return;

    const int columns = std::max(1, m_columns);
    const size_t rows = m_rows > 0 ? static_cast<size_t>(m_rows) : (visibleCount + static_cast<size_t>(columns) - 1) / static_cast<size_t>(columns);
    const size_t gridRows = std::max<size_t>(1, rows);

    const Vector3 size = getComponent<Transform>()->getSize();
    const float cellWidth = std::max(1.0f, (size.x - static_cast<float>(columns - 1) * m_spacing) / static_cast<float>(columns));
    const float cellHeight = std::max(1.0f, (size.y - static_cast<float>(gridRows - 1) * m_spacing) / static_cast<float>(gridRows));

    size_t index = 0;
    for (const auto& child : m_children) {
        if (!child->getVisible())
            continue;
        auto childTransform = child->getComponent<Transform>();
        if (!childTransform)
            continue;
        const size_t column = index % static_cast<size_t>(columns);
        const size_t row = index / static_cast<size_t>(columns);
        childTransform->setSize(cellWidth, cellHeight);
        childTransform->setPosition(static_cast<float>(column) * (cellWidth + m_spacing), static_cast<float>(row) * (cellHeight + m_spacing), 0.0f);
        ++index;
    }
}

} // namespace morrow
