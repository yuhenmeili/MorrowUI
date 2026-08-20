#ifndef MORROW_GUI_MRMENUBUTTON_H
#define MORROW_GUI_MRMENUBUTTON_H

#include "MRButton.h"
#include "MRPopupMenu.h"

namespace morrow {

class MRMenuButton : public MRButton {
public:
    /// 菜单项选中回调。
    using SelectionCallback = MRPopupMenu::ItemSelectedCallback;

    /// 创建一个菜单按钮。
    static std::shared_ptr<MRMenuButton> create();

    /// 设置按钮使用的弹出菜单。
    void setPopupMenu(const MRPopupMenuSharedPtr& menu);

    /// 获取按钮当前使用的弹出菜单。
    MRPopupMenuSharedPtr getPopupMenu() const {
        return m_menu;
    }
    /// 向弹出菜单添加菜单项；id 小于 0 时自动生成。
    void addMenuItem(const std::wstring& text, int id = -1);

    /// 设置菜单项选中回调。
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
