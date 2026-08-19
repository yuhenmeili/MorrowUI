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
    struct Item {
        int id = 0;
        std::wstring text;
        bool enabled = true;
    };

    using ItemSelectedCallback = std::function<void(int, const std::wstring&)>;

    static std::shared_ptr<MRPopupMenu> create();

    void addItem(const std::wstring& text, int id = -1);

    void addItem(const std::wstring& text, int id, bool enabled);

    void clear();

    void setItemHeight(float height);

    void setMenuWidth(float width);

    void setOnItemSelectedCallback(ItemSelectedCallback callback);

    void popup(float x, float y);

    void popupBelow(const Math::Rect& anchorBounds);

    void attachTo(const std::shared_ptr<Widget>& root);

    void hide();

    bool isOpen() const {
        return m_open;
    }

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
