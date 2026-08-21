#include "MRItemList.h"

#include <algorithm>

#include "base/Interaction.h"
#include "base/TouchEvent.h"
#include "base/Transform.h"

namespace morrow {

std::shared_ptr<MRItemList> MRItemList::create() {
    auto list = std::shared_ptr<MRItemList>(new MRItemList());
    list->addComponent<Interaction>();
    list->attachWheel(list);
    return list;
}

MRItemList::MRItemList() : UIWidget(false) {
    setWidgetType("MRItemList");
    getComponent<Transform>()->addSizeChangeListener([this]() { layoutItems(); });
}

void MRItemList::addItem(const std::wstring& text, int id, bool enabled) {
    if (id < 0)
        id = static_cast<int>(m_items.size());
    const size_t index = m_items.size();
    m_items.push_back({id, text, enabled});

    auto button = MRButton::create();
    button->setText(text, "default");
    button->setTextFontSize(20.0f);
    button->setTextAlign(HorizontalAlignment::LEFT, VerticalAlignment::CENTER);
    button->setCornerRadius(5.0f);
    button->setEnabled(enabled);
    m_itemClickConnections.emplace_back(button->events().onClicked.connect([this, index](BaseButton&) { selectIndex(index, true); }));
    addChild(button);
    attachWheel(button);
    m_itemButtons.push_back(button);
    updateItemStyle(index);
    layoutItems();
}

void MRItemList::clear() {
    for (const auto& button : m_itemButtons)
        removeChild(button);
    m_items.clear();
    m_itemButtons.clear();
    m_itemClickConnections.clear();
    m_wheelConnections.erase(std::remove_if(m_wheelConnections.begin(), m_wheelConnections.end(), [](const EventConnection& connection) { return !connection.connected(); }),
                             m_wheelConnections.end());
    m_selectedIndex = -1;
    m_scrollOffset = 0.0f;
    m_maxScrollOffset = 0.0f;
}

void MRItemList::setItemHeight(float height) {
    m_itemHeight = std::max(1.0f, height);
    layoutItems();
}

void MRItemList::setItemSpacing(float spacing) {
    m_itemSpacing = std::max(0.0f, spacing);
    layoutItems();
}

void MRItemList::setScrollOffset(float offset) {
    const float clamped = std::clamp(offset, 0.0f, m_maxScrollOffset);
    if (m_scrollOffset == clamped)
        return;
    m_scrollOffset = clamped;
    layoutItems();
    requestRender("item list scroll");
}

float MRItemList::getScrollOffset() const {
    return m_scrollOffset;
}

float MRItemList::getMaxScrollOffset() const {
    return m_maxScrollOffset;
}

bool MRItemList::selectItem(int id) {
    for (size_t i = 0; i < m_items.size(); ++i) {
        if (m_items[i].id == id && m_items[i].enabled) {
            selectIndex(i, true);
            return true;
        }
    }
    return false;
}

int MRItemList::getSelectedId() const {
    if (m_selectedIndex < 0 || static_cast<size_t>(m_selectedIndex) >= m_items.size())
        return -1;
    return m_items[static_cast<size_t>(m_selectedIndex)].id;
}

MRItemList::Events& MRItemList::events() {
    return m_events;
}

const std::vector<MRItemList::Item>& MRItemList::getItems() const {
    return m_items;
}

void MRItemList::update(FrameStateSharedPtr frameState) {
    layoutItems();
    UIWidget::update(frameState);
}

void MRItemList::selectIndex(size_t index, bool notify) {
    if (index >= m_items.size() || !m_items[index].enabled)
        return;
    const int previous = m_selectedIndex;
    m_selectedIndex = static_cast<int>(index);
    if (previous >= 0)
        updateItemStyle(static_cast<size_t>(previous));
    updateItemStyle(index);
    if (notify) {
        m_events.onItemSelected.notify(*this, m_items[index].id, m_items[index].text);
    }
}

void MRItemList::layoutItems() {
    const Vector3 size = getComponent<Transform>()->getSize();
    const float contentHeight = m_items.empty() ? 0.0f : static_cast<float>(m_items.size()) * m_itemHeight + static_cast<float>(m_items.size() - 1) * m_itemSpacing;
    m_maxScrollOffset = std::max(0.0f, contentHeight - size.y);
    m_scrollOffset = std::clamp(m_scrollOffset, 0.0f, m_maxScrollOffset);

    for (size_t i = 0; i < m_itemButtons.size(); ++i) {
        const float y = static_cast<float>(i) * (m_itemHeight + m_itemSpacing) - m_scrollOffset;
        auto transform = m_itemButtons[i]->getComponent<Transform>();
        transform->setPosition(0.0f, y, 0.0f);
        transform->setSize(size.x, m_itemHeight);
        m_itemButtons[i]->setVisible(y + m_itemHeight >= 0.0f && y <= size.y);
    }
}

void MRItemList::attachWheel(const std::shared_ptr<Widget>& widget) {
    auto interaction = widget ? widget->getComponent<Interaction>() : nullptr;
    if (!interaction)
        return;
    m_wheelConnections.emplace_back(
        interaction->addEventListener(TOUCH_EVENT_TYPE_WHEEL, [this](TouchEvent& event) { setScrollOffset(m_scrollOffset - event.wheelDeltaY * m_itemHeight); }));
}

void MRItemList::updateItemStyle(size_t index) {
    if (index >= m_itemButtons.size())
        return;
    auto& button = m_itemButtons[index];
    const bool selected = static_cast<int>(index) == m_selectedIndex;
    button->setTextColor(selected ? Vector4(1.0f, 1.0f, 1.0f, 1.0f) : Vector4(0.12f, 0.16f, 0.22f, 1.0f));
    button->setBackgroundColor(selected ? Vector4(0.12f, 0.48f, 0.72f, 1.0f) : Vector4(0.91f, 0.94f, 0.97f, 1.0f));
    button->setHoverColor(selected ? Vector4(0.1f, 0.42f, 0.66f, 1.0f) : Vector4(0.82f, 0.88f, 0.94f, 1.0f));
    button->setPressedColor(Vector4(0.08f, 0.36f, 0.58f, 1.0f));
}

}  // namespace morrow
