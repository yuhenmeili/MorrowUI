#include "MRTree.h"

#include <algorithm>

#include "base/Interaction.h"
#include "base/TouchEvent.h"
#include "base/Transform.h"

namespace morrow {

std::shared_ptr<MRTree> MRTree::create() {
    auto tree = std::shared_ptr<MRTree>(new MRTree());
    tree->addComponent<Interaction>();
    tree->attachWheel(tree);
    return tree;
}

MRTree::MRTree() : UIWidget(false) {
    setWidgetType("MRTree");
    getComponent<Transform>()->addSizeChangeListener([this]() { layoutRows(); });
}

bool MRTree::addNode(int id, const std::wstring& text, int parentId, bool expanded, bool enabled) {
    if (findNodeIndex(id) >= 0 || (parentId >= 0 && findNodeIndex(parentId) < 0))
        return false;
    m_nodes.push_back({id, parentId, text, expanded, enabled});
    m_treeDirty = true;
    rebuildVisibleNodes();
    return true;
}

void MRTree::clear() {
    for (const auto& row : m_rows)
        removeChild(row);
    m_nodes.clear();
    m_visibleNodes.clear();
    m_rows.clear();
    m_selectedIndex = -1;
    m_scrollOffset = 0.0f;
    m_maxScrollOffset = 0.0f;
    m_treeDirty = false;
}

void MRTree::setRowHeight(float height) {
    m_rowHeight = std::max(1.0f, height);
    layoutRows();
}

void MRTree::setIndentWidth(float width) {
    m_indentWidth = std::max(0.0f, width);
    layoutRows();
}

void MRTree::setRowStyle(const RowStyle& style) {
    m_rowStyle = style;
    layoutRows();
    requestRender("tree row style");
}

void MRTree::setScrollOffset(float offset) {
    const float clamped = std::clamp(offset, 0.0f, m_maxScrollOffset);
    if (m_scrollOffset == clamped)
        return;
    m_scrollOffset = clamped;
    layoutRows();
    requestRender("tree scroll");
}

bool MRTree::setExpanded(int id, bool expanded) {
    const int index = findNodeIndex(id);
    if (index < 0 || !hasChildren(id))
        return false;
    auto& node = m_nodes[static_cast<size_t>(index)];
    if (node.expanded == expanded)
        return true;
    node.expanded = expanded;
    m_treeDirty = true;
    rebuildVisibleNodes();
    m_events.onNodeExpanded.notify(*this, id, expanded);
    return true;
}

bool MRTree::toggleExpanded(int id) {
    const int index = findNodeIndex(id);
    return index >= 0 && setExpanded(id, !m_nodes[static_cast<size_t>(index)].expanded);
}

bool MRTree::isExpanded(int id) const {
    const int index = findNodeIndex(id);
    return index >= 0 && m_nodes[static_cast<size_t>(index)].expanded;
}

bool MRTree::selectNode(int id) {
    const int index = findNodeIndex(id);
    if (index < 0 || !m_nodes[static_cast<size_t>(index)].enabled)
        return false;
    selectIndex(static_cast<size_t>(index), true);
    return true;
}

int MRTree::getSelectedId() const {
    if (m_selectedIndex < 0 || static_cast<size_t>(m_selectedIndex) >= m_nodes.size())
        return -1;
    return m_nodes[static_cast<size_t>(m_selectedIndex)].id;
}

MRTree::Events& MRTree::events() {
    return m_events;
}

const std::vector<MRTree::Node>& MRTree::getNodes() const {
    return m_nodes;
}

void MRTree::update(FrameStateSharedPtr frameState) {
    if (m_treeDirty)
        rebuildVisibleNodes();
    layoutRows();
    UIWidget::update(frameState);
}

int MRTree::findNodeIndex(int id) const {
    for (size_t i = 0; i < m_nodes.size(); ++i) {
        if (m_nodes[i].id == id)
            return static_cast<int>(i);
    }
    return -1;
}

bool MRTree::hasChildren(int id) const {
    return std::any_of(m_nodes.begin(), m_nodes.end(), [id](const Node& node) { return node.parentId == id; });
}

void MRTree::rebuildVisibleNodes() {
    m_visibleNodes.clear();
    for (size_t i = 0; i < m_nodes.size(); ++i) {
        if (m_nodes[i].parentId == -1) {
            m_visibleNodes.push_back({i, 0});
            if (m_nodes[i].expanded)
                appendVisibleChildren(m_nodes[i].id, 1);
        }
    }

    while (m_rows.size() < m_visibleNodes.size()) {
        auto row = MRButton::create();
        row->setTextFontSize(19.0f);
        row->setTextAlign(HorizontalAlignment::LEFT, VerticalAlignment::CENTER);
        row->setCornerRadius(4.0f);
        addChild(row);
        attachWheel(row);
        m_rows.push_back(row);
        m_rowClickConnections.emplace_back();
    }
    for (size_t i = 0; i < m_rows.size(); ++i)
        m_rows[i]->setVisible(i < m_visibleNodes.size());
    for (size_t i = 0; i < m_visibleNodes.size(); ++i) {
        const size_t nodeIndex = m_visibleNodes[i].nodeIndex;
        const bool parent = hasChildren(m_nodes[nodeIndex].id);
        m_rowClickConnections[i] = m_rows[i]->events().onClicked.connect([this, nodeIndex, parent](BaseButton&) {
            selectIndex(nodeIndex, true);
            if (parent) {
                toggleExpanded(m_nodes[nodeIndex].id);
            }
        });
    }
    m_treeDirty = false;
    layoutRows();
}

void MRTree::appendVisibleChildren(int parentId, int depth) {
    for (size_t i = 0; i < m_nodes.size(); ++i) {
        if (m_nodes[i].parentId != parentId)
            continue;
        m_visibleNodes.push_back({i, depth});
        if (m_nodes[i].expanded)
            appendVisibleChildren(m_nodes[i].id, depth + 1);
    }
}

void MRTree::layoutRows() {
    const Vector3 size = getComponent<Transform>()->getSize();
    const float contentHeight = static_cast<float>(m_visibleNodes.size()) * m_rowHeight;
    m_maxScrollOffset = std::max(0.0f, contentHeight - size.y);
    m_scrollOffset = std::clamp(m_scrollOffset, 0.0f, m_maxScrollOffset);

    for (size_t i = 0; i < m_visibleNodes.size(); ++i) {
        configureRow(i, m_visibleNodes[i]);
        const float x = static_cast<float>(m_visibleNodes[i].depth) * m_indentWidth;
        const float y = static_cast<float>(i) * m_rowHeight - m_scrollOffset;
        auto transform = m_rows[i]->getComponent<Transform>();
        transform->setPosition(x, y, 0.0f);
        transform->setSize(std::max(1.0f, size.x - x), m_rowHeight - 4.0f);
        m_rows[i]->setVisible(y + m_rowHeight >= 0.0f && y <= size.y);
    }
}

void MRTree::selectIndex(size_t nodeIndex, bool notify) {
    if (nodeIndex >= m_nodes.size() || !m_nodes[nodeIndex].enabled)
        return;
    m_selectedIndex = static_cast<int>(nodeIndex);
    layoutRows();
    if (notify) {
        m_events.onNodeSelected.notify(*this, m_nodes[nodeIndex].id, m_nodes[nodeIndex].text);
    }
}

void MRTree::attachWheel(const std::shared_ptr<Widget>& widget) {
    auto interaction = widget ? widget->getComponent<Interaction>() : nullptr;
    if (!interaction)
        return;
    m_wheelConnections.emplace_back(
        interaction->addEventListener(TOUCH_EVENT_TYPE_WHEEL, [this](TouchEvent& event) { setScrollOffset(m_scrollOffset - event.wheelDeltaY * m_rowHeight); }));
}

void MRTree::configureRow(size_t rowIndex, const VisibleNode& visibleNode) {
    auto& node = m_nodes[visibleNode.nodeIndex];
    auto& row = m_rows[rowIndex];
    const bool parent = hasChildren(node.id);
    const std::wstring prefix = parent ? (node.expanded ? L"[-] " : L"[+] ") : L"    ";
    row->setText(prefix + node.text, "default");
    row->setEnabled(node.enabled);
    const bool selected = static_cast<int>(visibleNode.nodeIndex) == m_selectedIndex;
    row->setTextColor(selected ? m_rowStyle.selectedTextColor : m_rowStyle.textColor);
    row->setBackgroundColor(selected ? m_rowStyle.selectedBackgroundColor : m_rowStyle.backgroundColor);
    row->setHoverColor(selected ? m_rowStyle.selectedHoverColor : m_rowStyle.hoverColor);
    row->setPressedColor(m_rowStyle.pressedColor);
}

}  // namespace morrow
