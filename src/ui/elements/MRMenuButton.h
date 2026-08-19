#ifndef MORROW_GUI_MRMENUBUTTON_H
#define MORROW_GUI_MRMENUBUTTON_H

#include "MRButton.h"
#include "MRPopupMenu.h"

namespace morrow {

class MRMenuButton : public MRButton {
public:
    using SelectionCallback = MRPopupMenu::ItemSelectedCallback;

    static std::shared_ptr<MRMenuButton> create();

    void setPopupMenu(const MRPopupMenuSharedPtr& menu);

    MRPopupMenuSharedPtr getPopupMenu() const {
        return m_menu;
    }
    void addMenuItem(const std::wstring& text, int id = -1);

    void setOnMenuItemSelectedCallback(SelectionCallback callback);

protected:
    MRMenuButton();

    void onActivated() override;

private:
    MRPopupMenuSharedPtr m_menu;
    SelectionCallback m_onSelected;
};

using MRMenuButtonSharedPtr = std::shared_ptr<MRMenuButton>;

}  // namespace morrow

#endif  // MORROW_GUI_MRMENUBUTTON_H
