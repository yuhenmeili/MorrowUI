#include "MRMenuButton.h"

namespace morrow {

std::shared_ptr<MRMenuButton> MRMenuButton::create() {
    return std::shared_ptr<MRMenuButton>(new MRMenuButton());
}

MRMenuButton::MRMenuButton() {
    setWidgetType("MRMenuButton");
}

void MRMenuButton::setPopupMenu(const MRPopupMenuSharedPtr& menu) {
    m_menuSelectionConnection.disconnect();
    m_menu = menu;
    if (!m_menu)
        return;
    auto weakSelf = std::weak_ptr<MRMenuButton>(std::static_pointer_cast<MRMenuButton>(shared_from_this()));
    m_menuSelectionConnection =
        m_menu->events().onItemSelected.connect(
            [weakSelf](MRPopupMenu&, int id, const std::wstring& text) {
        if (auto self = weakSelf.lock())
            self->m_menuEvents.onItemSelected.notify(*self, id, text);
    });
}

void MRMenuButton::addMenuItem(const std::wstring& text, int id) {
    if (!m_menu)
        setPopupMenu(MRPopupMenu::create());
    m_menu->addItem(text, id);
}

MRMenuButton::MenuEvents& MRMenuButton::menuEvents() {
    return m_menuEvents;
}

void MRMenuButton::onActivated() {
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

}  // namespace morrow
