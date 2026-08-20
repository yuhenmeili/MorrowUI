#ifndef MORROW_GUI_MRPOPUPMENU_H
#define MORROW_GUI_MRPOPUPMENU_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "MRButton.h"
#include "MRColor.h"

namespace morrow {

class MRPopupMenu : public UIWidget {
public:
    /// 弹出菜单项数据。
    struct Item {
        /// 菜单项标识。
        int id = 0;
        /// 菜单项显示文字。
        std::wstring text;
        /// 菜单项是否可用。
        bool enabled = true;
    };

    /// 菜单项选中回调，参数依次为菜单项 id 和文字。
    using ItemSelectedCallback = std::function<void(int, const std::wstring&)>;

    /// 创建一个弹出菜单。
    static std::shared_ptr<MRPopupMenu> create();

    /// 添加一个可用菜单项；id 小于 0 时自动生成。
    void addItem(const std::wstring& text, int id = -1);

    /// 添加菜单项并指定是否可用。
    void addItem(const std::wstring& text, int id, bool enabled);

    /// 删除全部菜单项。
    void clear();

    /// 设置每个菜单项的高度。
    void setItemHeight(float height);

    /// 设置菜单宽度。
    void setMenuWidth(float width);

    /// 设置菜单项选中回调。
    void setOnItemSelectedCallback(ItemSelectedCallback callback);

    /// 在根节点坐标中的指定位置显示菜单。
    void popup(float x, float y);

    /// 在指定锚点区域下方显示菜单。
    void popupBelow(const Math::Rect& anchorBounds);

    /// 将菜单挂载到指定根节点。
    void attachTo(const std::shared_ptr<Widget>& root);

    /// 隐藏菜单。
    void hide();

    /// 获取菜单当前是否正在显示。
    bool isOpen() const {
        return m_open;
    }

    /// 获取只读菜单项数据。
    const std::vector<Item>& getItems() const {
        return m_items;
    }

private:
    MRPopupMenu();

    void selectItem(size_t index);

    void layoutItems();

    MRColorSharedPtr m_background;
    std::vector<Item> m_items;
    std::vector<std::shared_ptr<MRButton>> m_itemButtons;
    ItemSelectedCallback m_onItemSelected;
    float m_itemHeight = 42.0f;
    float m_menuWidth = 240.0f;
    bool m_open = false;
};

using MRPopupMenuSharedPtr = std::shared_ptr<MRPopupMenu>;

}  // namespace morrow

#endif  // MORROW_GUI_MRPOPUPMENU_H
