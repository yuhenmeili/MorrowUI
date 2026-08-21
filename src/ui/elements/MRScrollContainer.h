#ifndef MORROW_GUI_MRSCROLLCONTAINER_H
#define MORROW_GUI_MRSCROLLCONTAINER_H

#include <memory>
#include <vector>

#include "MRScrollBar.h"

namespace morrow {

class MRScrollContainer : public UIWidget {
public:
    /// 创建一个支持垂直滚动的内容容器。
    static std::shared_ptr<MRScrollContainer> create();

    /// 设置容器的滚动内容节点。
    void setContent(const std::shared_ptr<UIWidget>& content);
    /// 获取当前滚动内容节点。
    std::shared_ptr<UIWidget> getContent() const {
        return m_content;
    }
    /// 向内容节点添加子节点，并为其绑定滚动输入。
    void addScrollChild(const std::shared_ptr<Widget>& child);

    /// 设置滚轮每次滚动的距离。
    void setScrollStep(float step);
    /// 获取当前垂直滚动偏移。
    float getScrollOffset() const {
        return m_scrollOffset;
    }
    /// 获取最大垂直滚动偏移。
    float getMaxScrollOffset() const {
        return m_maxScrollOffset;
    }
    /// 设置垂直滚动偏移，取值会限制在有效范围内。
    void setScrollOffset(float offset);

    /// 获取容器使用的垂直滚动条。
    MRScrollBarSharedPtr getVerticalScrollBar() const {
        return m_scrollBar;
    }
    /// 每帧刷新内容范围、滚动条和子节点可见性。
    void update(FrameStateSharedPtr frameState) override;

private:
    MRScrollContainer();

    void attachInput(const std::shared_ptr<Widget>& widget);
    void attachInputRecursive(const std::shared_ptr<Widget>& widget);
    void scrollBy(float amount);
    void refreshMetrics();
    void updateChildVisibility();

    std::shared_ptr<UIWidget> m_content;
    MRScrollBarSharedPtr m_scrollBar;
    Observable<MRScrollBar&, float>::Connection m_scrollBarConnection;
    std::vector<EventConnection> m_inputConnections;
    float m_scrollOffset = 0.0f;
    float m_maxScrollOffset = 0.0f;
    float m_scrollStep = 48.0f;
    bool m_metricsDirty = true;
    bool m_dragging = false;
    float m_lastDragY = 0.0f;
};

using MRScrollContainerSharedPtr = std::shared_ptr<MRScrollContainer>;

}  // namespace morrow

#endif  // MORROW_GUI_MRSCROLLCONTAINER_H
