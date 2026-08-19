#ifndef MORROW_GUI_MRITEMLIST_H
#define MORROW_GUI_MRITEMLIST_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "MRButton.h"
#include "base/UIWidget.h"

namespace morrow {

/// 可滚动的单选列表控件。
class MRItemList : public UIWidget {
public:
    struct Item {
        int id = 0;
        std::wstring text;
        bool enabled = true;
    };

    using ItemSelectedCallback = std::function<void(int, const std::wstring&)>;

    /// 创建列表控件。
    static std::shared_ptr<MRItemList> create();
    /// 添加一项；id 小于 0 时自动使用当前索引。
    void addItem(const std::wstring& text, int id = -1, bool enabled = true);
    /// 删除全部列表项。
    void clear();
    /// 设置每一项的高度。
    void setItemHeight(float height);
    /// 设置列表项之间的间距。
    void setItemSpacing(float spacing);
    /// 设置垂直滚动偏移。
    void setScrollOffset(float offset);
    /// 获取当前垂直滚动偏移。
    float getScrollOffset() const;
    /// 获取最大垂直滚动偏移。
    float getMaxScrollOffset() const;
    /// 按 id 选中列表项。
    bool selectItem(int id);
    /// 获取当前选中项 id；未选中时返回 -1。
    int getSelectedId() const;
    /// 设置列表项选中回调。
    void setOnItemSelectedCallback(ItemSelectedCallback callback);
    /// 获取只读列表数据。
    const std::vector<Item>& getItems() const;
    void update(FrameStateSharedPtr frameState) override;

private:
    MRItemList();

    void selectIndex(size_t index, bool notify);

    void layoutItems();

    void attachWheel(const std::shared_ptr<Widget>& widget);

    void updateItemStyle(size_t index);

    std::vector<Item> m_items;
    std::vector<std::shared_ptr<MRButton>> m_itemButtons;
    ItemSelectedCallback m_onItemSelected;
    float m_itemHeight = 48.0f;
    float m_itemSpacing = 6.0f;
    float m_scrollOffset = 0.0f;
    float m_maxScrollOffset = 0.0f;
    int m_selectedIndex = -1;
};

using MRItemListSharedPtr = std::shared_ptr<MRItemList>;
using ItemList = MRItemList;

}  // namespace morrow

#endif  // MORROW_GUI_MRITEMLIST_H
