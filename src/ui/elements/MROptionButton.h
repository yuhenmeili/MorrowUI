#ifndef MORROW_GUI_MROPTIONBUTTON_H
#define MORROW_GUI_MROPTIONBUTTON_H

#include "MRButton.h"
#include "MRPopupMenu.h"

namespace morrow {

class MROptionButton : public MRButton {
public:
    /// 选项选中回调。
    using SelectionCallback = MRPopupMenu::ItemSelectedCallback;

    /// 创建一个下拉选项按钮。
    static std::shared_ptr<MROptionButton> create();

    /// 设置按钮使用的弹出菜单。
    void setPopupMenu(const MRPopupMenuSharedPtr& menu);

    /// 获取按钮当前使用的弹出菜单。
    MRPopupMenuSharedPtr getPopupMenu() const {
        return m_menu;
    }

    /// 向弹出菜单添加选项；id 小于 0 时自动生成。
    void addOption(const std::wstring& text, int id = -1);

    /// 删除全部选项并清除当前选择。
    void clearOptions();

    /// 按 id 选中一个选项。
    void setSelected(int id);

    /// 获取当前选中项 id；未选中时返回 -1。
    int getSelectedId() const {
        return m_selectedId;
    }
    /// 获取当前选中项文字。
    const std::wstring& getSelectedText() const {
        return m_selectedText;
    }

    /// 设置选项选中回调。
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
