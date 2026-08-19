#ifndef MORROW_GUI_MROPTIONBUTTON_H
#define MORROW_GUI_MROPTIONBUTTON_H

#include "MRButton.h"
#include "MRPopupMenu.h"

namespace morrow {

class MROptionButton : public MRButton {
public:
    using SelectionCallback = MRPopupMenu::ItemSelectedCallback;

    static std::shared_ptr<MROptionButton> create();

    void setPopupMenu(const MRPopupMenuSharedPtr& menu);

    MRPopupMenuSharedPtr getPopupMenu() const {
        return m_menu;
    }

    void addOption(const std::wstring& text, int id = -1);

    void clearOptions();

    void setSelected(int id);

    int getSelectedId() const {
        return m_selectedId;
    }
    const std::wstring& getSelectedText() const {
        return m_selectedText;
    }

    void setOnSelectedCallback(SelectionCallback callback);

protected:
    MROptionButton();

    void onActivated() override;

private:
    void handleSelection(int id, const std::wstring& text);

    MRPopupMenuSharedPtr m_menu;
    SelectionCallback m_onSelected;
    int m_selectedId = -1;
    std::wstring m_selectedText;
};

using MROptionButtonSharedPtr = std::shared_ptr<MROptionButton>;

}  // namespace morrow

#endif  // MORROW_GUI_MROPTIONBUTTON_H
