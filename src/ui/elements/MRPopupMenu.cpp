#include "MRPopupMenu.h"

#include <algorithm>

#include "base/Transform.h"

namespace morrow {

std::shared_ptr<MRPopupMenu> MRPopupMenu::create() {
    auto menu = std::shared_ptr<MRPopupMenu>(new MRPopupMenu());
    menu->addChild(menu->m_background);
    return menu;
}

MRPopupMenu::MRPopupMenu() : UIWidget(false) {
    setWidgetType("MRPopupMenu");
    setDisplayLayer(10);
    m_background = MRColor::create();
    m_background->setColor(0.98f, 0.99f, 1.0f, 1.0f);
    m_background->setRounding(6.0f);
    setVisible(false);
}

void MRPopupMenu::addItem(const std::wstring& text, int id) {
    addItem(text, id, true);
}

void MRPopupMenu::addItem(const std::wstring& text, int id, bool enabled) {
    if (id < 0)
        id = static_cast<int>(m_items.size());
    const size_t index = m_items.size();
    m_items.push_back({id, text, enabled});

    auto button = MRButton::create();
    button->setText(text, "default");
    button->setTextFontSize(20.0f);
    button->setTextColor(0.1f, 0.12f, 0.16f, 1.0f);
    button->setTextAlign(HorizontalAlignment::LEFT, VerticalAlignment::CENTER);
    button->setBackgroundColor(0.98f, 0.99f, 1.0f, 1.0f);
    button->setHoverColor(Vector4(0.84f, 0.9f, 0.96f, 1.0f));
    button->setPressedColor(Vector4(0.74f, 0.84f, 0.93f, 1.0f));
    button->setCornerRadius(4.0f);
    button->setEnabled(enabled);
    m_itemClickConnections.emplace_back(
        button->events().onClicked.connect(
            [this, index](BaseButton&) { selectItem(index); }));
    addChild(button);
    m_itemButtons.push_back(button);
    layoutItems();
}

void MRPopupMenu::clear() {
    for (const auto& button : m_itemButtons) {
        removeChild(button);
    }
    m_items.clear();
    m_itemButtons.clear();
    m_itemClickConnections.clear();
    layoutItems();
}

void MRPopupMenu::setItemHeight(float height) {
    m_itemHeight = std::max(1.0f, height);
    layoutItems();
}

void MRPopupMenu::setMenuWidth(float width) {
    m_menuWidth = std::max(1.0f, width);
    layoutItems();
}

MRPopupMenu::Events& MRPopupMenu::events() {
    return m_events;
}

void MRPopupMenu::popup(float x, float y) {
    if (!m_parent)
        return;
    if (const auto parent = std::dynamic_pointer_cast<UIWidget>(m_parent)) {
        const Math::Rect parentBounds = parent->getScreenSpaceAABB();
        const Vector3 menuSize = getComponent<Transform>()->getSize();
        x = std::clamp(x, parentBounds.Min.x, std::max(parentBounds.Min.x, parentBounds.Max.x - menuSize.x));
        y = std::clamp(y, parentBounds.Min.y, std::max(parentBounds.Min.y, parentBounds.Max.y - menuSize.y));
        x -= parentBounds.Min.x;
        y -= parentBounds.Min.y;
    }
    getComponent<Transform>()->setPosition(x, y, 0.0f);
    setVisible(true);
    m_open = true;
    requestRender("popup");
}

void MRPopupMenu::popupBelow(const Math::Rect& anchorBounds) {
    float y = anchorBounds.Max.y + 4.0f;
    if (const auto parent = std::dynamic_pointer_cast<UIWidget>(m_parent)) {
        const Math::Rect parentBounds = parent->getScreenSpaceAABB();
        const float menuHeight = getComponent<Transform>()->getSize().y;
        if (y + menuHeight > parentBounds.Max.y)
            y = anchorBounds.Min.y - menuHeight - 4.0f;
    }
    popup(anchorBounds.Min.x, y);
}

void MRPopupMenu::attachTo(const std::shared_ptr<Widget>& root) {
    if (!root || root.get() == this)
        return;
    if (m_parent != root)
        root->addChild(shared_from_this());
}

void MRPopupMenu::hide() {
    m_open = false;
    setVisible(false);
}

void MRPopupMenu::selectItem(size_t index) {
    if (index >= m_items.size() || !m_items[index].enabled)
        return;
    const Item selected = m_items[index];
    hide();
    m_events.onItemSelected.notify(*this, selected.id, selected.text);
}

void MRPopupMenu::layoutItems() {
    const float height = m_itemHeight * static_cast<float>(m_items.size());
    getComponent<Transform>()->setSize(m_menuWidth, height);
    m_background->getComponent<Transform>()->setPosition(0.0f, 0.0f, -0.1f);
    m_background->getComponent<Transform>()->setSize(m_menuWidth, height);

    for (size_t i = 0; i < m_itemButtons.size(); ++i) {
        auto transform = m_itemButtons[i]->getComponent<Transform>();
        transform->setPosition(6.0f, 4.0f + static_cast<float>(i) * m_itemHeight, 0.0f);
        transform->setSize(m_menuWidth - 12.0f, m_itemHeight - 8.0f);
    }
}

}  // namespace morrow
