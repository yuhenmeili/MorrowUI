#include "MROptionButton.h"

#include "base/Transform.h"

namespace morrow {

std::shared_ptr<MROptionButton> MROptionButton::create() {
    return std::shared_ptr<MROptionButton>(new MROptionButton());
}

MROptionButton::MROptionButton() {
    setWidgetType("MROptionButton");
    setTextAlign(HorizontalAlignment::LEFT, VerticalAlignment::CENTER);
}

void MROptionButton::setPopupMenu(const MRPopupMenuSharedPtr& menu) {
    m_menuSelectionConnection.disconnect();
    m_menu = menu;
    if (!m_menu)
        return;
    auto weakSelf = std::weak_ptr<MROptionButton>(std::static_pointer_cast<MROptionButton>(shared_from_this()));
    m_menuSelectionConnection =
        m_menu->events().onItemSelected.connect(
            [weakSelf](MRPopupMenu&, int id, const std::wstring& text) {
        if (auto self = weakSelf.lock())
            self->handleSelection(id, text);
    });
}

void MROptionButton::addOption(const std::wstring& text, int id) {
    if (!m_menu) {
        setPopupMenu(MRPopupMenu::create());
    }
    m_menu->addItem(text, id);
}

void MROptionButton::clearOptions() {
    if (m_menu)
        m_menu->clear();
    m_selectedId = -1;
    m_selectedText.clear();
}

void MROptionButton::setSelected(int id) {
    if (!m_menu)
        return;
    for (const auto& item : m_menu->getItems()) {
        if (item.id == id) {
            handleSelection(item.id, item.text);
            return;
        }
    }
}

MROptionButton::SelectionEvents& MROptionButton::selectionEvents() {
    return m_selectionEvents;
}

void MROptionButton::onActivated() {
    if (!m_menu || !m_parent)
        return;
    if (m_menu->isOpen()) {
        m_menu->hide();
        return;
    }
    auto root = shared_from_this();
    while (root->m_parent)
        root = root->m_parent;
    m_menu->attachTo(root);
    m_menu->popupBelow(getScreenSpaceAABB());
}

void MROptionButton::handleSelection(int id, const std::wstring& text) {
    m_selectedId = id;
    m_selectedText = text;
    setText(text, "default");
    m_selectionEvents.onSelected.notify(*this, id, text);
}

}  // namespace morrow
